#include "hepch.h"
#include "RenderTextBatches.h"

/*
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ComputeTextBatches.h"
#include "Heart/Renderer/Plugins/RenderMaterialBatches.h"
#include "Heart/Renderer/Plugins/EntityIds.h"
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
    void RenderTextBatches::Initialize()
    {
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
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 }
            }
        });
        if (eidPlugin)
        {
            rpCreateInfo.ColorAttachments.push_back({ eidPlugin->GetTexture()->GetColorFormat() });
            rpCreateInfo.Subpasses.back().OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 1 });
        }
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_material_batches/Text.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = {{ false }};
        if (eidPlugin)
            pipelineCreateInfo.BlendStates.push_back({ false });
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("text", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        // TODO: store this in text?
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_TextResourceSet = pipeline->CreateResourceSet(2, dsCreateInfo);

        RenderMaterialBatches::PBRConfigData initialConfig;
        initialConfig.SSAOEnable = false;
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(RenderMaterialBatches::PBRConfigData);
        bufCreateInfo.ElementCount = 1;
        bufCreateInfo.InitialData = &initialConfig;
        bufCreateInfo.InitialDataSize = sizeof(initialConfig);
        m_DataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        ResizeInternal();
    }

    void RenderTextBatches::ResizeInternal()
    {
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        if (eidPlugin)
            fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, eidPlugin->GetTexture() });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get());
    }

    void RenderTextBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderTextBatches");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto batchesPlugin = m_Renderer->GetPlugin<RenderPlugins::ComputeTextBatches>(m_Info.TextBatchesPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        const auto& computedData = batchesPlugin->GetComputedData();

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
        m_ResourceSet->BindTextureLayer(8, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        m_ResourceSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("text");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);

        for (auto& batch : computedData.Batches)
        {
            // Bind material
            encoder->BindResourceSet(batch.Material->GetResourceSet(), 1);
            encoder->FlushResourceSet(1);

            // Bind atlas
            m_TextResourceSet->BindTexture(0, batch.FontAtlas);
            m_TextResourceSet->FlushBindings();
            encoder->BindResourceSet(m_TextResourceSet.get(), 2);
            encoder->FlushResourceSet(2);

            // Bind mesh
            encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());

            // Draw
            encoder->DrawIndexedIndirect(
                computedData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }

        encoder->EndEncoding();
    }
}
*/
