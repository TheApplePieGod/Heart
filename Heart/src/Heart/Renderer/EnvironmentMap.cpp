#include "hepch.h"
#include "EnvironmentMap.h"

#include "Heart/Core/App.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Camera.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Events/AppEvents.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/GraphicsCommandEncoder.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/GraphicsPipeline.h"
#include "Flourish/Api/Buffer.h"

namespace Heart
{
    EnvironmentMap::EnvironmentMap(UUID mapAsset)
        : m_MapAsset(mapAsset)
    {
        SubscribeToEmitter(&App::Get());
        Initialize();
    }

    EnvironmentMap::~EnvironmentMap()
    {
        UnsubscribeFromEmitter(&App::Get());
        Shutdown();
    }

    void EnvironmentMap::OnEvent(Event& event)
    {
        event.Map<AppGraphicsInitEvent>(HE_BIND_EVENT_FN(EnvironmentMap::OnAppGraphicsInit));
        event.Map<AppGraphicsShutdownEvent>(HE_BIND_EVENT_FN(EnvironmentMap::OnAppGraphicsShutdown));
    }

    bool EnvironmentMap::OnAppGraphicsInit(AppGraphicsInitEvent& event)
    {
        if (!m_Initialized)
        {
            Initialize();
            if (m_MapAsset)
                Recalculate();
        }
        return false;
    }

    bool EnvironmentMap::OnAppGraphicsShutdown(AppGraphicsShutdownEvent& event)
    {
        Shutdown();
        return false;
    }
    
    void EnvironmentMap::Initialize()
    {
        m_Initialized = true;

        // Create texture & cubemap targets
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = 512;
        texCreateInfo.Height = 512;
        texCreateInfo.Channels = 4;
        texCreateInfo.DataType = Flourish::BufferDataType::HalfFloat;
        texCreateInfo.UsageType = Flourish::BufferUsageType::Static;
        texCreateInfo.RenderTarget = true;
        texCreateInfo.ArrayCount = 6;
        texCreateInfo.MipCount = 0;
        m_EnvironmentMap.Texture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        m_BRDFTexture.Texture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.ArrayCount = 6;
        m_IrradianceMap.Texture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.MipCount = 5;
        m_PrefilterMaps.Resize(texCreateInfo.MipCount);
        m_PrefilterMaps[0].Texture = Flourish::Texture::Create(texCreateInfo);

        // Create the command buffers
        Flourish::CommandBufferCreateInfo cmdCreateInfo;
        cmdCreateInfo.MaxEncoders = 2;
        m_BRDFTexture.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        m_EnvironmentMap.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        m_IrradianceMap.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        for (u32 i = 0; i < m_PrefilterMaps.Count(); i++)
            m_PrefilterMaps[i].CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);

        // Create the cubemap data buffer to hold data for each face render
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Layout = {
            { Flourish::BufferDataType::Mat4 },
            { Flourish::BufferDataType::Mat4 },
            { Flourish::BufferDataType::Float4 }
        };
        bufCreateInfo.ElementCount = 100;
        m_CubemapDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        // Create renderpass
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        for (u32 i = 0; i < m_EnvironmentMap.Texture->GetArrayCount(); i++)
        {
            rpCreateInfo.ColorAttachments.push_back({ m_EnvironmentMap.Texture->GetColorFormat() });
            rpCreateInfo.Subpasses.push_back({ {}, { { Flourish::SubpassAttachmentType::Color, i } } });
        }
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        // Create BRDF renderpass
        rpCreateInfo.ColorAttachments = { { m_BRDFTexture.Texture->GetColorFormat() } };
        rpCreateInfo.Subpasses = { { {}, { { Flourish::SubpassAttachmentType::Color, 0 } } } };
        m_BRDFRenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        std::array<float, 4> clearColor = { 0.f, 0.f, 0.f, 0.f };

        // ------------------------------------------------------------------
        // Environment map framebuffer: convert loaded image into a cubemap
        // ------------------------------------------------------------------
        {
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_RenderPass;
            fbCreateInfo.Width = m_EnvironmentMap.Texture->GetWidth();
            fbCreateInfo.Height = m_EnvironmentMap.Texture->GetHeight();
            for (u32 i = 0; i < m_EnvironmentMap.Texture->GetArrayCount(); i++)
                fbCreateInfo.ColorAttachments.push_back({ clearColor, m_EnvironmentMap.Texture, i, 0 });
            m_EnvironmentMap.Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/EnvironmentMap.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/CalcEnvironmentMap.frag", true)->GetShader();
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            m_RenderPass->CreatePipeline("cubemap", pipelineCreateInfo);
        }

        // -----------------------------------------------------------------------------------------
        // Irradiance framebuffer: calculate environment's irradiance and store it in a cubemap
        // ---------------------------------------------------------------------------------------
        {
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_RenderPass;
            fbCreateInfo.Width = m_IrradianceMap.Texture->GetWidth();
            fbCreateInfo.Height = m_IrradianceMap.Texture->GetHeight();
            for (u32 i = 0; i < m_IrradianceMap.Texture->GetArrayCount(); i++)
                fbCreateInfo.ColorAttachments.push_back({ clearColor, m_IrradianceMap.Texture, i, 0 });
            m_IrradianceMap.Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/EnvironmentMap.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/CalcIrradianceMap.frag", true)->GetShader();
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            m_RenderPass->CreatePipeline("irradiance", pipelineCreateInfo);
        }

        // -----------------------------------------------------------------------------------------------------------------------
        // Prefilter framebuffers: calculate environment's light contribution based on roughness and store it in a cubemap's mips
        // ----------------------------------------------------------------------------------------------------------------------
        {
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_RenderPass;

            for (u32 i = 0; i < m_PrefilterMaps[0].Texture->GetMipCount(); i++) // each mip level
            {
                fbCreateInfo.ColorAttachments.clear();
                fbCreateInfo.Width = m_PrefilterMaps[0].Texture->GetMipWidth(i);
                fbCreateInfo.Height = m_PrefilterMaps[0].Texture->GetMipHeight(i);
                for (u32 j = 0; j < m_PrefilterMaps[0].Texture->GetArrayCount(); j++)
                    fbCreateInfo.ColorAttachments.push_back({ clearColor, m_PrefilterMaps[0].Texture, j, i });
                m_PrefilterMaps[i].Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);
            }

            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/EnvironmentMap.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/CalcPrefilterMap.frag", true)->GetShader();
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            m_RenderPass->CreatePipeline("prefilter", pipelineCreateInfo);
        }

        // -----------------------------------------------------------------------------------------------------------------------
        // BRDF framebuffer: solve the BRDF integral and store it in a texture (TODO: store this somewhere because it is constant)
        // ----------------------------------------------------------------------------------------------------------------------
        {
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_BRDFRenderPass;
            fbCreateInfo.Width = m_BRDFTexture.Texture->GetWidth();
            fbCreateInfo.Height = m_BRDFTexture.Texture->GetHeight();
            fbCreateInfo.ColorAttachments.push_back({ clearColor, m_BRDFTexture.Texture, 0, 0 });
            m_BRDFTexture.Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/BRDFQuad.vert", true)->GetShader();
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/CalcBRDF.frag", true)->GetShader();
            pipelineCreateInfo.VertexInput = false;
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthTest = false;
            pipelineCreateInfo.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            m_BRDFRenderPass->CreatePipeline("brdf", pipelineCreateInfo);
        }
    }

    void EnvironmentMap::Shutdown()
    {
        m_Initialized = false;

        m_RenderPass.reset();
        m_BRDFRenderPass.reset();

        m_EnvironmentMap = RenderData();
        m_IrradianceMap = RenderData();
        m_BRDFTexture = RenderData();

        for (auto& data : m_PrefilterMaps)
            data = RenderData();

        m_CubemapDataBuffer.reset();
        m_FrameDataBuffer.reset();
    }

    void EnvironmentMap::Recalculate()
    {
        auto loadTimer = Timer("Environment map generation");

        // Retrieve the basic cube mesh
        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        // Retrieve the associated env map asset
        auto mapAsset = AssetManager::RetrieveAsset<TextureAsset>(m_MapAsset);
        if (!mapAsset || !mapAsset->IsValid())
        {
            HE_ENGINE_LOG_ERROR("Could not calculate environment map, cannot retrieve map asset");
            return;
        }

        // +X -X +Y -Y +Z -Z rotations for rendering each face of the cubemap
        glm::vec3 rotations[] = {
            { 0.f, 90.f, 0.f },
            { 0.f, -90.f, 0.f },
            { -90.f, 0.f, 0.f },
            { 90.f, 0.f, 0.f },
            { 0.f, 0.f, 0.f },
            { 0.f, 180.f, 0.f }
        };

        // The camera that will be used to render each face
        Camera cubemapCam(90.f, 0.1f, 50.f, 1.f);

        // Dynamic offset into the CubeData buffer
        u32 cubeDataIndex = 0;

        // ------------------------------------------------------------------
        // Render equirectangular map to cubemap
        // ------------------------------------------------------------------
        {
            auto rcEncoder = m_EnvironmentMap.CommandBuffer->EncodeRenderCommands(m_EnvironmentMap.Framebuffer.get());
            for (u32 i = 0; i < 6; i++)
            {
                if (i != 0)
                    rcEncoder->StartNextSubpass();

                rcEncoder->BindPipeline("cubemap");  

                cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[i]);

                CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f) };
                m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
                rcEncoder->BindPipelineBufferResource(0, m_CubemapDataBuffer.get(), 0, i, 1);
                rcEncoder->BindPipelineTextureResource(1, mapAsset->GetTexture());
                rcEncoder->FlushPipelineBindings();

                rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                rcEncoder->DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                cubeDataIndex++;
            }
            rcEncoder->EndEncoding();

            auto gEncoder = m_EnvironmentMap.CommandBuffer->EncodeGraphicsCommands();
            gEncoder->GenerateMipMaps(m_EnvironmentMap.Texture.get());
            gEncoder->EndEncoding();
        }

        // ------------------------------------------------------------------
        // Precalculate environment irradiance
        // ------------------------------------------------------------------
        {
            auto rcEncoder = m_IrradianceMap.CommandBuffer->EncodeRenderCommands(m_IrradianceMap.Framebuffer.get());
            for (u32 i = 0; i < 6; i++)
            {
                if (i != 0)
                    rcEncoder->StartNextSubpass();

                rcEncoder->BindPipeline("irradiance");  
                rcEncoder->BindPipelineBufferResource(0, m_CubemapDataBuffer.get(), 0, i, 1);
                rcEncoder->BindPipelineTextureResource(1, m_EnvironmentMap.Texture.get());
                rcEncoder->FlushPipelineBindings();

                rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                rcEncoder->DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );
            }
            rcEncoder->EndEncoding();
        }

        // ------------------------------------------------------------------
        // Prefilter the environment map based on roughness
        // ------------------------------------------------------------------
        for (u32 i = 0; i < m_PrefilterMaps.Count(); i++)
        {
            auto rcEncoder = m_PrefilterMaps[i].CommandBuffer->EncodeRenderCommands(m_PrefilterMaps[i].Framebuffer.get());

            float roughness = static_cast<float>(i) / 4;
            for (u32 j = 0; j < 6; j++) // each face
            {
                if (j != 0)
                    rcEncoder->StartNextSubpass();

                rcEncoder->BindPipeline("prefilter");  

                cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[j]);

                CubemapData mapData = {
                    cubemapCam.GetProjectionMatrix(),
                    cubemapCam.GetViewMatrix(),
                    glm::vec4(roughness, m_EnvironmentMap.Texture->GetWidth(), 0.f, 0.f)
                };
                m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
                rcEncoder->BindPipelineBufferResource(0, m_CubemapDataBuffer.get(), 0, cubeDataIndex, 1);
                rcEncoder->BindPipelineTextureResource(1, m_EnvironmentMap.Texture.get());
                rcEncoder->FlushPipelineBindings();

                rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                rcEncoder->DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                cubeDataIndex++;
            }
        }

        // ------------------------------------------------------------------
        // Precalculate the BRDF texture
        // ------------------------------------------------------------------
        {
            auto rcEncoder = m_BRDFTexture.CommandBuffer->EncodeRenderCommands(m_BRDFTexture.Framebuffer.get());

            rcEncoder->BindPipeline("brdf");  

            CubemapData mapData = {
                cubemapCam.GetProjectionMatrix(),
                cubemapCam.GetViewMatrix(),
                glm::vec4(Flourish::Context::ReversedZBuffer(), 0.f, 0.f, 0.f)
            };
            m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
            rcEncoder->BindPipelineBufferResource(0, m_CubemapDataBuffer.get(), 0, cubeDataIndex, 1);
            rcEncoder->FlushPipelineBindings();

            rcEncoder->Draw(3, 0, 1);

            cubeDataIndex++;
        }

        std::vector<std::vector<Flourish::CommandBuffer*>> submission = {
            { m_EnvironmentMap.CommandBuffer.get() }, { m_IrradianceMap.CommandBuffer.get() }
        };
        for (u32 i = 0; i < m_PrefilterMaps.Count(); i++)
            submission[1].push_back(m_PrefilterMaps[i].CommandBuffer.get());

        Flourish::Context::SubmitCommandBuffers({ { m_BRDFTexture.CommandBuffer.get() } });
        Flourish::Context::SubmitCommandBuffers(submission);
    }
}