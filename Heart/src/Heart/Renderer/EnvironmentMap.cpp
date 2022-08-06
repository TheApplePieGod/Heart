#include "hepch.h"
#include "EnvironmentMap.h"

#include "Heart/Core/App.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Core/Camera.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Events/AppEvents.h"

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
        m_EnvironmentMap = Texture::Create({ 512, 512, 4, BufferDataType::HalfFloat, BufferUsageType::Static, 6, 0 });
        m_IrradianceMap = Texture::Create({ 256, 256, 4, BufferDataType::HalfFloat, BufferUsageType::Static, 6, 1 });
        m_PrefilterMap = Texture::Create({ 256, 256, 4, BufferDataType::HalfFloat, BufferUsageType::Static, 6, 5 });
        m_BRDFTexture = Texture::Create({ 512, 512, 4, BufferDataType::HalfFloat, BufferUsageType::Static, 1, 1 });

        // Create the cubemap data buffer to hold data for each face render
        BufferLayout cubemapDataLayout = {
            { BufferDataType::Mat4 },
            { BufferDataType::Mat4 },
            { BufferDataType::Float4 }
        };
        m_CubemapDataBuffer = Buffer::Create(Buffer::Type::Storage, BufferUsageType::Dynamic, cubemapDataLayout, 100, nullptr);

        // ------------------------------------------------------------------
        // Cubemap framebuffer: convert loaded image into a cubemap
        // ------------------------------------------------------------------
        FramebufferCreateInfo cubemapFbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 0 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 1 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 2 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 3 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 4 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_EnvironmentMap, 5 }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            m_EnvironmentMap->GetWidth(), m_EnvironmentMap->GetHeight(),
            MsaaSampleCount::None
        };
        m_CubemapFramebuffer = Framebuffer::Create(cubemapFbCreateInfo);

        GraphicsPipelineCreateInfo envPipeline = {
            AssetManager::GetAssetUUID("engine/EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("engine/CalcEnvironmentMap.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        for (u32 i = 0; i < 6; i++) // create a pipeline for each subpass
        {
            envPipeline.SubpassIndex = i;
            m_CubemapFramebuffer->RegisterGraphicsPipeline(std::to_string(i), envPipeline);
        }

        // -----------------------------------------------------------------------------------------
        // Irradiance framebuffer: calculate environment's irradiance and store it in a cubemap
        // ---------------------------------------------------------------------------------------
        FramebufferCreateInfo irradianceFbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 0 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 1 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 2 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 3 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 4 },
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_IrradianceMap, 5 }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            m_IrradianceMap->GetWidth(), m_IrradianceMap->GetHeight(),
            MsaaSampleCount::None
        };
        m_IrradianceMapFramebuffer = Framebuffer::Create(irradianceFbCreateInfo);

        GraphicsPipelineCreateInfo irradiancePipeline = {
            AssetManager::GetAssetUUID("engine/EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("engine/CalcIrradianceMap.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        for (u32 i = 0; i < 6; i++) // pipeline for each subpass
        {
            irradiancePipeline.SubpassIndex = i;
            m_IrradianceMapFramebuffer->RegisterGraphicsPipeline(std::to_string(i), irradiancePipeline);
        }

        // -----------------------------------------------------------------------------------------------------------------------
        // Prefilter framebuffers: calculate environment's light contribution based on roughness and store it in a cubemap's mips
        // ----------------------------------------------------------------------------------------------------------------------
        FramebufferCreateInfo prefilterFbCreateInfo = {
            {}, {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } },
                { {}, { { SubpassAttachmentType::Color, 1 } } },
                { {}, { { SubpassAttachmentType::Color, 2 } } },
                { {}, { { SubpassAttachmentType::Color, 3 } } },
                { {}, { { SubpassAttachmentType::Color, 4 } } },
                { {}, { { SubpassAttachmentType::Color, 5 } } }
            },
            0, 0,
            MsaaSampleCount::None
        };

        GraphicsPipelineCreateInfo prefilterPipeline = {
            AssetManager::GetAssetUUID("engine/EnvironmentMap.vert", true),
            AssetManager::GetAssetUUID("engine/CalcPrefilterMap.frag", true),
            true,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        for (u32 i = 0; i < 5; i++) // each mip level
        {
            prefilterFbCreateInfo.ColorAttachments.Clear();
            for (u32 j = 0; j < 6; j++) // each face
                prefilterFbCreateInfo.ColorAttachments.Add({ { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_PrefilterMap, j, i });
            prefilterFbCreateInfo.Width = static_cast<u32>(m_PrefilterMap->GetWidth() * pow(0.5f, i));
            prefilterFbCreateInfo.Height = static_cast<u32>(m_PrefilterMap->GetHeight() * pow(0.5f, i));

            m_PrefilterFramebuffers.AddInPlace(Framebuffer::Create(prefilterFbCreateInfo));
            for (u32 j = 0; j < 6; j++) // each face
            {
                prefilterPipeline.SubpassIndex = j;
                m_PrefilterFramebuffers.Back()->RegisterGraphicsPipeline(std::to_string(j), prefilterPipeline);
            }
        }

        // -----------------------------------------------------------------------------------------------------------------------
        // BRDF framebuffer: solve the BRDF integral and store it in a texture (TODO: store this somewhere because it is constant)
        // ----------------------------------------------------------------------------------------------------------------------
        FramebufferCreateInfo brdfFbCreateInfo = {
            {
                { { 0.f, 0.f, 0.f, 0.f }, false, ColorFormat::None, m_BRDFTexture }
            },
            {},
            {
                { {}, { { SubpassAttachmentType::Color, 0 } } }
            },
            m_BRDFTexture->GetWidth(), m_BRDFTexture->GetHeight(),
            MsaaSampleCount::None
        };
        m_BRDFFramebuffer = Framebuffer::Create(brdfFbCreateInfo);

        GraphicsPipelineCreateInfo brdfPipeline = {
            AssetManager::GetAssetUUID("engine/BRDFQuad.vert", true),
            AssetManager::GetAssetUUID("engine/CalcBRDF.frag", true),
            false,
            VertexTopology::TriangleList,
            Heart::Mesh::GetVertexLayout(),
            { { false } },
            false,
            false,
            CullMode::None,
            WindingOrder::Clockwise,
            0
        };
        m_BRDFFramebuffer->RegisterGraphicsPipeline("0", brdfPipeline);
    }

    void EnvironmentMap::Shutdown()
    {
        m_Initialized = false;

        m_EnvironmentMap.reset();
        m_IrradianceMap.reset();
        m_PrefilterMap.reset();
        m_BRDFTexture.reset();

        for (auto& buffer : m_PrefilterFramebuffers)
            buffer.reset();
        m_PrefilterFramebuffers.Clear();
        m_BRDFFramebuffer.reset();
        m_CubemapFramebuffer.reset();
        m_IrradianceMapFramebuffer.reset();

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
        m_CubemapFramebuffer->Bind();
        for (u32 i = 0; i < 6; i++)
        {
            if (i != 0)
                m_CubemapFramebuffer->StartNextSubpass();

            m_CubemapFramebuffer->BindPipeline(std::to_string(i));  

            cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[i]);

            CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f) };
            m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
            m_CubemapFramebuffer->BindShaderBufferResource(0, cubeDataIndex, 1, m_CubemapDataBuffer.get());

            m_CubemapFramebuffer->BindShaderTextureResource(1, mapAsset->GetTexture());

            m_CubemapFramebuffer->FlushBindings();

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            cubeDataIndex++;
        }

        m_EnvironmentMap->RegenerateMipMapsSync(m_CubemapFramebuffer.get());

        Renderer::Api().RenderFramebuffers(Window::GetMainWindow().GetContext(), { { m_CubemapFramebuffer.get() } });

        // ------------------------------------------------------------------
        // Precalculate environment irradiance
        // ------------------------------------------------------------------
        m_IrradianceMapFramebuffer->Bind();
        for (u32 i = 0; i < 6; i++)
        {
            if (i != 0)
                m_IrradianceMapFramebuffer->StartNextSubpass();

            m_IrradianceMapFramebuffer->BindPipeline(std::to_string(i));  

            cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[i]);

            CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f) };
            m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
            m_IrradianceMapFramebuffer->BindShaderBufferResource(0, cubeDataIndex, 1, m_CubemapDataBuffer.get());

            m_IrradianceMapFramebuffer->BindShaderTextureResource(1, m_EnvironmentMap.get());

            m_IrradianceMapFramebuffer->FlushBindings();

            Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
            Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
            Renderer::Api().DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                0, 0, 1
            );

            cubeDataIndex++;
        }

        Renderer::Api().RenderFramebuffers(Window::GetMainWindow().GetContext(), { { m_IrradianceMapFramebuffer.get() } });

        // ------------------------------------------------------------------
        // Prefilter the environment map based on roughness
        // ------------------------------------------------------------------
        for (u32 i = 0; i < m_PrefilterFramebuffers.Count(); i++)
        {
            m_PrefilterFramebuffers[i]->Bind();

            float roughness = static_cast<float>(i) / 4;
            for (u32 j = 0; j < 6; j++) // each face
            {
                if (j != 0)
                    m_PrefilterFramebuffers[i]->StartNextSubpass();

                m_PrefilterFramebuffers[i]->BindPipeline(std::to_string(j));  

                cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[i]);

                CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(roughness, m_EnvironmentMap->GetWidth(), 0.f, 0.f) };
                m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
                m_PrefilterFramebuffers[i]->BindShaderBufferResource(0, cubeDataIndex, 1, m_CubemapDataBuffer.get());

                m_PrefilterFramebuffers[i]->BindShaderTextureResource(1, m_EnvironmentMap.get());

                m_PrefilterFramebuffers[i]->FlushBindings();

                Renderer::Api().BindVertexBuffer(*meshData.GetVertexBuffer());
                Renderer::Api().BindIndexBuffer(*meshData.GetIndexBuffer());
                Renderer::Api().DrawIndexed(
                    meshData.GetIndexBuffer()->GetAllocatedCount(),
                    0, 0, 1
                );

                cubeDataIndex++;
            }

            Renderer::Api().RenderFramebuffers(Window::GetMainWindow().GetContext(), { { m_PrefilterFramebuffers[i].get() } });
        }

        // ------------------------------------------------------------------
        // Precalculate the BRDF texture
        // ------------------------------------------------------------------
        m_BRDFFramebuffer->Bind();
        m_BRDFFramebuffer->BindPipeline("0");  

        CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(Renderer::IsUsingReverseDepth(), 0.f, 0.f, 0.f) };
        m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
        m_BRDFFramebuffer->BindShaderBufferResource(0, cubeDataIndex, 1, m_CubemapDataBuffer.get());
        cubeDataIndex++;

        m_BRDFFramebuffer->FlushBindings();

        Renderer::Api().Draw(3, 0, 1);
        Renderer::Api().RenderFramebuffers(Window::GetMainWindow().GetContext(), { { m_BRDFFramebuffer.get() } });
    }
}