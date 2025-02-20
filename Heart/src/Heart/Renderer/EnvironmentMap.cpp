#include "hepch.h"
#include "EnvironmentMap.h"

#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Camera.h"
#include "Heart/Core/Window.h"
#include "Heart/Core/Timing.h"
#include "Heart/Task/TaskManager.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/GraphicsCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/GraphicsPipeline.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/RenderGraph.h"

namespace Heart
{
    EnvironmentMap::EnvironmentMap(UUID mapAsset)
        : m_MapAsset(mapAsset)
    {
        Initialize();
    }

    EnvironmentMap::~EnvironmentMap()
    {

    }

    void EnvironmentMap::Initialize()
    {
        // Queue shader loads 
        auto mapVertShader = AssetManager::RetrieveAsset("engine/EnvironmentMap.vert", true);
        auto envMapFrag = AssetManager::RetrieveAsset("engine/CalcEnvironmentMap.frag", true);
        auto irrMapFrag = AssetManager::RetrieveAsset("engine/CalcIrradianceMap.frag", true);
        auto prefilterMapFrag = AssetManager::RetrieveAsset("engine/CalcPrefilterMap.frag", true);
        auto brdfVert = AssetManager::RetrieveAsset("engine/BRDFQuad.vert", true);
        auto brdfFrag = AssetManager::RetrieveAsset("engine/CalcBRDF.frag", true);
        Asset::LoadMany(
            { mapVertShader, envMapFrag, irrMapFrag, prefilterMapFrag, brdfVert, brdfFrag },
            false
        );

        // Create texture & cubemap targets
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = 512;
        texCreateInfo.Height = 512;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        texCreateInfo.ArrayCount = 6;
        texCreateInfo.MipCount = 0;
        texCreateInfo.SamplerState.UVWWrap = {
            Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge
        };
        m_EnvironmentMap.Texture = Flourish::Texture::Create(texCreateInfo);
        if (m_GenerateBRDF)
        {
            texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
            texCreateInfo.Format = Flourish::ColorFormat::RG16_FLOAT;
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            m_BRDFTexture.Texture = Flourish::Texture::Create(texCreateInfo);
        }
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.MipCount = 1;
        texCreateInfo.ArrayCount = 6;
        m_IrradianceMap.Texture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.MipCount = 5;
        m_PrefilterMaps.Resize(texCreateInfo.MipCount);
        m_PrefilterMaps[0].Texture = Flourish::Texture::Create(texCreateInfo);

        // Create the command buffers
        Flourish::CommandBufferCreateInfo cmdCreateInfo;
        cmdCreateInfo.FrameRestricted = false;
        m_BRDFTexture.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        m_EnvironmentMap.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        m_IrradianceMap.CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);
        for (u32 i = 0; i < m_PrefilterMaps.Count(); i++)
            m_PrefilterMaps[i].CommandBuffer = Flourish::CommandBuffer::Create(cmdCreateInfo);

        // Create the render graph
        Flourish::RenderGraphCreateInfo rgCreateInfo;
        rgCreateInfo.Usage = Flourish::RenderGraphUsageType::PerFrame;
        m_RenderGraph = Flourish::RenderGraph::Create(rgCreateInfo);

        // Create the cubemap data buffer to hold data for each face render
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPUWrite;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Storage;
        bufCreateInfo.Stride = sizeof(CubemapData);
        bufCreateInfo.ElementCount = 100;
        m_CubemapDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        // Create the CPU buffer to transfer BRDF texture data 
        if (m_GenerateBRDF)
        {
            bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPURead;
            bufCreateInfo.Usage = Flourish::BufferUsageFlags::Generic;
            bufCreateInfo.Stride = sizeof(u16);
            bufCreateInfo.ElementCount = m_BRDFTexture.Texture->GetWidth() * m_BRDFTexture.Texture->GetHeight() * 2;
            m_BRDFTexBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }

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
        if (m_GenerateBRDF)
        {
            rpCreateInfo.ColorAttachments = { { m_BRDFTexture.Texture->GetColorFormat() } };
            rpCreateInfo.Subpasses = { { {}, { { Flourish::SubpassAttachmentType::Color, 0 } } } };
            m_BRDFRenderPass = Flourish::RenderPass::Create(rpCreateInfo);
        }

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
            pipelineCreateInfo.VertexShader = { mapVertShader->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.FragmentShader = { envMapFrag->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthConfig.DepthTest = false;
            pipelineCreateInfo.DepthConfig.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            auto pipeline = m_RenderPass->CreatePipeline("cubemap", pipelineCreateInfo);

            Flourish::ResourceSetCreateInfo dsCreateInfo;
            m_EnvironmentMap.ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
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
            pipelineCreateInfo.VertexShader = { mapVertShader->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.FragmentShader = { irrMapFrag->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthConfig.DepthTest = false;
            pipelineCreateInfo.DepthConfig.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            auto pipeline = m_RenderPass->CreatePipeline("irradiance", pipelineCreateInfo);

            Flourish::ResourceSetCreateInfo dsCreateInfo;
            m_IrradianceMap.ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
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
            pipelineCreateInfo.VertexShader = { mapVertShader->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.FragmentShader = { prefilterMapFrag->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.VertexInput = true;
            pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
            pipelineCreateInfo.VertexLayout = Heart::Mesh::GetVertexLayout();
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthConfig.DepthTest = false;
            pipelineCreateInfo.DepthConfig.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::Frontface;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            auto pipeline = m_RenderPass->CreatePipeline("prefilter", pipelineCreateInfo);

            Flourish::ResourceSetCreateInfo dsCreateInfo;
            m_PrefilterMaps[0].ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
        }

        // -----------------------------------------------------------------------------------------------------------------------
        // BRDF framebuffer: solve the BRDF integral and store it in a texture (TODO: store this somewhere because it is constant)
        // ----------------------------------------------------------------------------------------------------------------------
        if (m_GenerateBRDF)
        {
            Flourish::FramebufferCreateInfo fbCreateInfo;
            fbCreateInfo.RenderPass = m_BRDFRenderPass;
            fbCreateInfo.Width = m_BRDFTexture.Texture->GetWidth();
            fbCreateInfo.Height = m_BRDFTexture.Texture->GetHeight();
            fbCreateInfo.ColorAttachments.push_back({ clearColor, m_BRDFTexture.Texture, 0, 0 });
            m_BRDFTexture.Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

            Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo.VertexShader = { brdfVert->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.FragmentShader = { brdfFrag->EnsureValid<ShaderAsset>()->GetShader() };
            pipelineCreateInfo.VertexInput = false;
            pipelineCreateInfo.BlendStates = { { false } };
            pipelineCreateInfo.DepthConfig.DepthTest = false;
            pipelineCreateInfo.DepthConfig.DepthWrite = false;
            pipelineCreateInfo.CullMode = Flourish::CullMode::None;
            pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;

            auto pipeline = m_BRDFRenderPass->CreatePipeline("brdf", pipelineCreateInfo);

            Flourish::ResourceSetCreateInfo dsCreateInfo;
            m_BRDFTexture.ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
        }
    }

    Task EnvironmentMap::Recalculate()
    {
        if (!m_MapAsset)
            return Task();

        Task task = TaskManager::Schedule(
            [this]()
            {
                auto loadTimer = Timer("Environment map generation");

                // Retrieve the basic cube mesh
                auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
                meshAsset->EnsureValid();
                auto& meshData = meshAsset->GetSubmesh(0);

                // Retrieve the associated env map asset
                auto mapAsset = AssetManager::RetrieveAsset<TextureAsset>(m_MapAsset);
                if (!mapAsset || !mapAsset->Load()->IsValid())
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
                    m_EnvironmentMap.ResourceSet->BindBuffer(0, m_CubemapDataBuffer.get(), 0, 1);
                    m_EnvironmentMap.ResourceSet->BindTexture(1, mapAsset->GetTexture());
                    m_EnvironmentMap.ResourceSet->FlushBindings();

                    auto rcEncoder = m_EnvironmentMap.CommandBuffer->EncodeRenderCommands(m_EnvironmentMap.Framebuffer.get());
                    for (u32 i = 0; i < 6; i++)
                    {
                        if (i != 0)
                            rcEncoder->StartNextSubpass();

                        cubemapCam.UpdateViewMatrix(glm::vec3(0.f), rotations[i]);

                        CubemapData mapData = { cubemapCam.GetProjectionMatrix(), cubemapCam.GetViewMatrix(), glm::vec4(0.f) };
                        m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);

                        rcEncoder->BindPipeline("cubemap");  
                        rcEncoder->BindResourceSet(m_EnvironmentMap.ResourceSet.get(), 0);
                        rcEncoder->UpdateDynamicOffset(0, 0, i * m_CubemapDataBuffer->GetStride());
                        rcEncoder->FlushResourceSet(0);

                        rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                        rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                        rcEncoder->DrawIndexed(
                            meshData.GetIndexBuffer()->GetAllocatedCount(),
                            0, 0, 1, 0
                        );

                        cubeDataIndex++;
                    }
                    rcEncoder->EndEncoding();

                    auto gEncoder = m_EnvironmentMap.CommandBuffer->EncodeGraphicsCommands();
                    gEncoder->GenerateMipMaps(m_EnvironmentMap.Texture.get(), Flourish::SamplerFilter::Linear);
                    gEncoder->EndEncoding();
                }

                // ------------------------------------------------------------------
                // Precalculate environment irradiance
                // ------------------------------------------------------------------
                {
                    if (!m_SetsWritten)
                    {
                        m_IrradianceMap.ResourceSet->BindBuffer(0, m_CubemapDataBuffer.get(), 0, 1);
                        m_IrradianceMap.ResourceSet->BindTexture(1, m_EnvironmentMap.Texture.get());
                        m_IrradianceMap.ResourceSet->FlushBindings();
                    }

                    auto rcEncoder = m_IrradianceMap.CommandBuffer->EncodeRenderCommands(m_IrradianceMap.Framebuffer.get());
                    for (u32 i = 0; i < 6; i++)
                    {
                        if (i != 0)
                            rcEncoder->StartNextSubpass();

                        rcEncoder->BindPipeline("irradiance");  
                        rcEncoder->BindResourceSet(m_IrradianceMap.ResourceSet.get(), 0);
                        rcEncoder->UpdateDynamicOffset(0, 0, i * m_CubemapDataBuffer->GetStride());
                        rcEncoder->FlushResourceSet(0);

                        rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                        rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                        rcEncoder->DrawIndexed(
                            meshData.GetIndexBuffer()->GetAllocatedCount(),
                            0, 0, 1, 0
                        );
                    }
                    rcEncoder->EndEncoding();
                }

                // ------------------------------------------------------------------
                // Prefilter the environment map based on roughness
                // ------------------------------------------------------------------
                if (!m_SetsWritten)
                {
                    m_PrefilterMaps[0].ResourceSet->BindBuffer(0, m_CubemapDataBuffer.get(), 0, 1);
                    m_PrefilterMaps[0].ResourceSet->BindTexture(1, m_EnvironmentMap.Texture.get());
                    m_PrefilterMaps[0].ResourceSet->FlushBindings();
                }

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
                        rcEncoder->BindResourceSet(m_PrefilterMaps[0].ResourceSet.get(), 0);
                        rcEncoder->UpdateDynamicOffset(0, 0, cubeDataIndex * m_CubemapDataBuffer->GetStride());
                        rcEncoder->FlushResourceSet(0);

                        rcEncoder->BindVertexBuffer(meshData.GetVertexBuffer());
                        rcEncoder->BindIndexBuffer(meshData.GetIndexBuffer());
                        rcEncoder->DrawIndexed(
                            meshData.GetIndexBuffer()->GetAllocatedCount(),
                            0, 0, 1, 0
                        );

                        cubeDataIndex++;
                    }

                    rcEncoder->EndEncoding();
                }

                // ------------------------------------------------------------------
                // Precalculate the BRDF texture
                // ------------------------------------------------------------------
                if (m_GenerateBRDF)
                {
                    if (!m_SetsWritten)
                    {
                        m_BRDFTexture.ResourceSet->BindBuffer(0, m_CubemapDataBuffer.get(), cubeDataIndex, 1);
                        m_BRDFTexture.ResourceSet->FlushBindings();
                    }

                    auto rcEncoder = m_BRDFTexture.CommandBuffer->EncodeRenderCommands(m_BRDFTexture.Framebuffer.get());

                    rcEncoder->BindPipeline("brdf");  

                    CubemapData mapData = {
                        cubemapCam.GetProjectionMatrix(),
                        cubemapCam.GetViewMatrix(),
                        glm::vec4(Flourish::Context::ReversedZBuffer(), 0.f, 0.f, 0.f)
                    };
                    m_CubemapDataBuffer->SetElements(&mapData, 1, cubeDataIndex);
                    rcEncoder->BindResourceSet(m_BRDFTexture.ResourceSet.get(), 0);
                    rcEncoder->FlushResourceSet(0);

                    rcEncoder->Draw(3, 0, 1, 0);

                    rcEncoder->EndEncoding();

                    auto xferEncoder = m_BRDFTexture.CommandBuffer->EncodeTransferCommands();
                    xferEncoder->CopyTextureToBuffer(m_BRDFTexture.Texture.get(), m_BRDFTexBuffer.get());
                    xferEncoder->FlushBuffer(m_BRDFTexBuffer.get());
                    xferEncoder->EndEncoding();

                    cubeDataIndex++;
                }

                m_SetsWritten = true;

                if (!m_RenderGraph->IsBuilt())
                {
                    m_RenderGraph->ConstructNewNode(m_EnvironmentMap.CommandBuffer.get())
                        .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                        .EncoderAddTextureWrite(m_EnvironmentMap.Texture.get())
                        .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                        .EncoderAddTextureRead(m_EnvironmentMap.Texture.get())
                        .EncoderAddTextureWrite(m_EnvironmentMap.Texture.get())
                        .AddToGraph();
                    m_RenderGraph->ConstructNewNode(m_IrradianceMap.CommandBuffer.get())
                        .AddExecutionDependency(m_EnvironmentMap.CommandBuffer.get())
                        .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                        .EncoderAddTextureWrite(m_IrradianceMap.Texture.get())
                        .EncoderAddTextureRead(m_EnvironmentMap.Texture.get())
                        .AddToGraph();
                    if (m_GenerateBRDF)
                    {
                        m_RenderGraph->ConstructNewNode(m_BRDFTexture.CommandBuffer.get())
                            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                            .EncoderAddTextureWrite(m_BRDFTexture.Texture.get())
                            .AddEncoderNode(Flourish::GPUWorkloadType::Transfer)
                            .EncoderAddTextureRead(m_BRDFTexture.Texture.get())
                            .EncoderAddBufferWrite(m_BRDFTexBuffer.get())
                            .AddToGraph();
                    }
                    for (u32 i = 0; i < m_PrefilterMaps.Count(); i++)
                    {
                        m_RenderGraph->ConstructNewNode(m_PrefilterMaps[i].CommandBuffer.get())
                            .AddExecutionDependency(m_EnvironmentMap.CommandBuffer.get())
                            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                            .EncoderAddTextureWrite(m_PrefilterMaps[0].Texture.get())
                            .EncoderAddTextureRead(m_EnvironmentMap.Texture.get())
                            .AddToGraph();
                    }
                    m_RenderGraph->Build();
                }

                if (!m_GenerateBRDF)
                    Flourish::Context::PushRenderGraph(m_RenderGraph.get());
                else
                {
                    Flourish::Context::ExecuteRenderGraph(m_RenderGraph.get());

                    u16* value = new u16[m_BRDFTexBuffer->GetAllocatedCount()];
                    m_BRDFTexBuffer->ReadBytes(value, m_BRDFTexBuffer->GetAllocatedSize(), 0);

                    TextureAsset::HeartTextureHeader header{};
                    header.Width = m_BRDFTexture.Texture->GetWidth();
                    header.Height = m_BRDFTexture.Texture->GetHeight();
                    header.Channels = 2;
                    header.DataType = 'f';
                    header.Precision = 2;
                    header.MipLevels = 1;

                    std::ofstream file("./BRDFTexture.hetex", std::ios::binary);
                    file.write((char*)&header, sizeof(TextureAsset::HeartTextureHeader));
                    file.write((char*)value, m_BRDFTexBuffer->GetAllocatedSize());
                    file.close();
                }
            },
            Task::Priority::Medium
        );

        // Wait here so that this GPU work gets scheduled first. This way frame rendering
        // will execute one this is done, so there arent frozen frames of strange outputs
        task.Wait();

        return task;
    }

    const Flourish::Texture* EnvironmentMap::GetBRDFTexture() const
    {
        return AssetManager::RetrieveAsset("engine/BRDFTexture.hetex", true)
            ->EnsureValid<TextureAsset>()
            ->GetTexture().get();
    }
}
