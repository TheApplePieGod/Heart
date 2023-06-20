#include "hepch.h"
#include "RenderMaterialBatches.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ComputeMaterialBatches.h"
#include "Heart/Renderer/Plugins/TransparencyComposite.h"
#include "Heart/Renderer/Plugins/EntityIds.h"
#include "Heart/Renderer/Plugins/SSAO.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/EnvironmentMap.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void RenderMaterialBatches::Initialize()
    {
        auto tpPlugin = m_Renderer->GetPlugin<RenderPlugins::TransparencyComposite>(m_Info.TransparencyCompositePluginName);
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

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
            Flourish::AttachmentInitialization::None
        });
        rpCreateInfo.ColorAttachments.push_back({
            tpPlugin->GetRevealTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::None
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
        if (eidPlugin)
        {
            rpCreateInfo.ColorAttachments.push_back({ eidPlugin->GetTexture()->GetColorFormat() });
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
        if (eidPlugin)
            pipelineCreateInfo.BlendStates.push_back({ false });
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("opaque", pipelineCreateInfo);

        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Transparent.frag", true)->GetShader();
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.BlendStates = {
            { false },
            { true, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One },
            { true, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcColor, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcAlpha }
        };
        if (eidPlugin)
            pipelineCreateInfo.BlendStates.push_back({ false });
        m_RenderPass->CreatePipeline("alpha", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(PBRConfigData);
        bufCreateInfo.ElementCount = 1;
        m_DataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        ResizeInternal();
    }

    void RenderMaterialBatches::ResizeInternal()
    {
        auto tpPlugin = m_Renderer->GetPlugin<RenderPlugins::TransparencyComposite>(m_Info.TransparencyCompositePluginName);
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, tpPlugin->GetAccumTexture() });
        fbCreateInfo.ColorAttachments.push_back({ { 1.f }, tpPlugin->GetRevealTexture() });
        if (eidPlugin)
            fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, eidPlugin->GetTexture() });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);
        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get())
            .EncoderAddTextureRead(ssaoPlugin->GetOutputTexture());
    }

    void RenderMaterialBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderMaterialBatches");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto batchesPlugin = m_Renderer->GetPlugin<RenderPlugins::ComputeMaterialBatches>(m_Info.MaterialBatchesPluginName);
        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        const auto& computedData = batchesPlugin->GetComputedData();

        PBRConfigData bufData = {
            data.Settings.SSAOEnable
        };
        m_DataBuffer->SetElements(&bufData, 1, 0);

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, computedData.ObjectDataBuffer.get(), 0, computedData.ObjectDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(2, computedData.MaterialDataBuffer.get(), 0, computedData.MaterialDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(3, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(4, m_DataBuffer.get(), 0, 1);
        if (data.EnvMap)
        {
            m_ResourceSet->BindTexture(5, data.EnvMap->GetIrradianceCubemap());
            m_ResourceSet->BindTexture(6, data.EnvMap->GetPrefilterCubemap());
            m_ResourceSet->BindTexture(7, data.EnvMap->GetBRDFTexture());
        }
        else
        {
            m_ResourceSet->BindTexture(5, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet->BindTexture(6, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet->BindTextureLayer(7, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        }
        m_ResourceSet->BindTextureLayer(8, ssaoPlugin->GetOutputTexture(), 0, 0);
        m_ResourceSet->FlushBindings();

        auto renderBatches = [&computedData](Flourish::RenderCommandEncoder* encoder, ComputeMaterialBatches::BatchType type)
        {
            auto& batchData = computedData.BatchTypes[(u32)type];
            Mesh* lastMesh = nullptr;
            for (auto& batch : batchData.Batches)
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
                    computedData.IndirectBuffer.get(), batch.First, batch.Count
                );
            }
        };

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());

        // Initial (opaque) batches pass
        encoder->BindPipeline("opaque");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        renderBatches(encoder, ComputeMaterialBatches::BatchType::Opaque);

        // Final alpha batches pass
        encoder->BindPipeline("alpha");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->ClearColorAttachment(1);
        encoder->ClearColorAttachment(2);
        renderBatches(encoder, ComputeMaterialBatches::BatchType::Alpha);

        encoder->EndEncoding();
    }
}
