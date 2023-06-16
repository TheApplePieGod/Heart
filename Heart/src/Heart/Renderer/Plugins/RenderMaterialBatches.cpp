#include "hepch.h"
#include "RenderMaterialBatches.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ComputeMaterialBatches.h"
#include "Heart/Renderer/Plugins/TransparencyComposite.h"
#include "Heart/Renderer/SceneRenderer2.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void RenderMaterialBatches::Initialize()
    {
        // Create default environment map cubemap object
        // TODO: this should be static somewhere (or an asset)
        {
            Flourish::TextureCreateInfo envTexCreateInfo;
            envTexCreateInfo.Width = 256;
            envTexCreateInfo.Height = 256;
            envTexCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
            envTexCreateInfo.Usage = Flourish::TextureUsageType::Readonly;
            envTexCreateInfo.Writability = Flourish::TextureWritability::Once;
            envTexCreateInfo.ArrayCount = 6;
            envTexCreateInfo.MipCount = 1;
            m_DefaultEnvironmentMap = Flourish::Texture::Create(envTexCreateInfo);
        }

        if (m_Info.CanOutputEntityIds)
        {
            Flourish::TextureCreateInfo texCreateInfo;
            texCreateInfo.Width = m_Renderer->GetRenderWidth();
            texCreateInfo.Height = m_Renderer->GetRenderHeight();
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
            texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
            texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
            texCreateInfo.Format = Flourish::ColorFormat::R32_FLOAT;
            m_EntityIdsTexture = Flourish::Texture::Create(texCreateInfo);
        }

        auto tpPlugin = m_Renderer->GetPlugin<RenderPlugins::TransparencyComposite>(m_Info.TransparencyCompositePluginName);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            m_Renderer->GetDepthTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_Renderer->GetRenderTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.ColorAttachments.push_back({
            tpPlugin->GetAccumTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.ColorAttachments.push_back({
            tpPlugin->GetRevealTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
                { Flourish::SubpassAttachmentType::Color, 1 },
                { Flourish::SubpassAttachmentType::Color, 2 }
            }
        });
        if (m_Info.CanOutputEntityIds)
        {
            rpCreateInfo.ColorAttachments.push_back({ m_EntityIdsTexture->GetColorFormat() });
            rpCreateInfo.Subpasses.back().OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 3 });
        }
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Opaque.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = {{ false }, { false }, { false }};
        if (m_Info.CanOutputEntityIds)
            pipelineCreateInfo.BlendStates.push_back({ false });
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("opaque", pipelineCreateInfo);

        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Transparent.frag", true)->GetShader();
        pipelineCreateInfo.BlendStates = {
            { false },
            { true, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One },
            { true, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcColor, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcAlpha }
        };
        if (m_Info.CanOutputEntityIds)
            pipelineCreateInfo.BlendStates.push_back({ false });
        m_RenderPass->CreatePipeline("alpha", pipelineCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, tpPlugin->GetAccumTexture() });
        fbCreateInfo.ColorAttachments.push_back({ { 1.f }, tpPlugin->GetRevealTexture() });
        if (m_Info.CanOutputEntityIds)
            fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, m_EntityIdsTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        if (m_Info.CanOutputEntityIds)
        {
            Flourish::BufferCreateInfo bufCreateInfo;
            bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
            bufCreateInfo.Type = Flourish::BufferType::Pixel;
            bufCreateInfo.Stride = sizeof(float);
            bufCreateInfo.ElementCount = m_Renderer->GetRenderWidth() * m_Renderer->GetRenderHeight();
            m_EntityIdsBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }

    void RenderMaterialBatches::ResizeInternal()
    {

    }

    void RenderMaterialBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderMaterialBatches");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto batchesPlugin = m_Renderer->GetPlugin<RenderPlugins::ComputeMaterialBatches>(m_Info.MaterialBatchesPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        const auto& batchData = batchesPlugin->GetBatchData();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, batchData.ObjectDataBuffer.get(), 0, batchData.ObjectDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(2, batchData.MaterialDataBuffer.get(), 0, batchData.MaterialDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(3, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        if (data.EnvMap)
        {
            m_ResourceSet->BindTexture(5, data.EnvMap->GetIrradianceCubemap());
            m_ResourceSet->BindTexture(6, data.EnvMap->GetPrefilterCubemap());
            m_ResourceSet->BindTexture(7, data.EnvMap->GetBRDFTexture());
        }
        else
        {
            m_ResourceSet->BindTexture(5, m_DefaultEnvironmentMap.get());
            m_ResourceSet->BindTexture(6, m_DefaultEnvironmentMap.get());
            m_ResourceSet->BindTextureLayer(7, m_DefaultEnvironmentMap.get(), 0, 0);
        }
        m_ResourceSet->BindTextureLayer(8, m_DefaultEnvironmentMap.get(), 0, 0);
        m_ResourceSet->FlushBindings();

        auto renderBatches = [&batchData](Flourish::RenderCommandEncoder* encoder, bool opaque)
        {
            auto& batches = opaque ? batchData.OpaqueBatches : batchData.AlphaBatches;
            Mesh* lastMesh = nullptr;
            for (auto& batch : batches)
            {
                // Bind material
                encoder->BindResourceSet(batch.Material->GetResourceSet(), 1);
                encoder->FlushResourceSet(1);

                // Bind mesh
                if (lastMesh != batch.Mesh)
                {
                    encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
                    encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());

                    lastMesh = batch.Mesh;
                }

                // Draw
                encoder->DrawIndexedIndirect(
                    batchData.IndirectBuffer.get(), batch.First, batch.Count
                );
            }
        };

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());

        // Initial (opaque) batches pass
        encoder->BindPipeline("opaque");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        renderBatches(encoder, true);

        // Final alpha batches pass
        encoder->BindPipeline("alpha");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        renderBatches(encoder, false);

        encoder->EndEncoding();

        if (m_Info.CanOutputEntityIds && data.Settings.CopyEntityIdsTextureToCPU)
        {
            auto encoder = m_CommandBuffer->EncodeTransferCommands();
            encoder->CopyTextureToBuffer(m_EntityIdsTexture.get(), m_EntityIdsBuffer.get());
            //encoder->FlushBuffer(m_EntityIdsBuffer.get());
            encoder->EndEncoding();

            // Lazy flushing because we don't care about immediate / accurate results
            m_EntityIdsBuffer->Flush();
        }
    }
}
