#include "hepch.h"
#include "Forward.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"
#include "Heart/Renderer/Plugins/ComputeTextBatches.h"
#include "Heart/Renderer/Plugins/EntityIds.h"
#include "Heart/Renderer/Plugins/CollectMaterials.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ClusteredLighting.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/GraphicsCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void Forward::InitializeInternal()
    {
        // Queue shader loads 
        auto textFrag = AssetManager::RetrieveAsset("engine/render_plugins/forward/Text.frag", true);
        auto vertShader = AssetManager::RetrieveAsset("engine/render_plugins/forward/Vertex.vert", true);
        auto standardFrag = AssetManager::RetrieveAsset("engine/render_plugins/forward/Standard.frag", true);
        Asset::LoadMany({ textFrag, vertShader, standardFrag }, false);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            Flourish::ColorFormat::Depth,
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_Info.OutputTexture->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve,
            (m_Info.OutputTexture->GetUsageType() & Flourish::TextureUsageFlags::Compute) > 0
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
            }
        });

        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = { standardFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.VertexShader = { vertShader->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        pipelineCreateInfo.AccessOverrides = {
            { 1, 0, Flourish::ShaderTypeFlags::All }
        };
        auto standardPipeline = m_RenderPass->CreatePipeline("standard", pipelineCreateInfo);
        pipelineCreateInfo.FragmentShader = { textFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        auto textPipeline = m_RenderPass->CreatePipeline("text", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        cbCreateInfo.MaxTimestamps = 3; // TODO: disable in dist
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_StandardResourceSet = standardPipeline->CreateResourceSet(0, dsCreateInfo);
        m_TextResourceSet = textPipeline->CreateResourceSet(2, dsCreateInfo);

        ResizeInternal();
    }

    void Forward::ResizeInternal()
    {
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Info.OutputTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_Info.DepthTexture });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get())
            .EncoderAddBufferRead(clusterPlugin->GetLightIndicesBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetLightGridBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetClusterDataBuffer());
    }

    void Forward::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::Forward");

        // Update timing stats
        m_Stats["GPU Time (Objects)"].Type = StatType::TimeMS;
        m_Stats["GPU Time (Objects)"].Data.Float = (float)(m_CommandBuffer->ComputeTimestampDifference(0, 1) * 1e-6);
        m_Stats["GPU Time (Text)"].Type = StatType::TimeMS;
        m_Stats["GPU Time (Text)"].Data.Float = (float)(m_CommandBuffer->ComputeTimestampDifference(1, 2) * 1e-6);

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        auto materialBuffer = materialsPlugin->GetMaterialBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);
        const auto& meshBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName)->GetBatchData();
        const auto& textBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeTextBatches>(m_Info.TextBatchesPluginName)->GetBatchData();

        m_StandardResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_StandardResourceSet->BindBuffer(1, meshBatchData.ObjectDataBuffer.get(), 0, meshBatchData.ObjectDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(2, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(3, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(4, clusterPlugin->GetLightIndicesBuffer(), 0, clusterPlugin->GetLightIndicesBuffer()->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(5, clusterPlugin->GetLightGridBuffer(), 0, clusterPlugin->GetLightGridBuffer()->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(6, clusterPlugin->GetClusterDataBuffer(), 0, clusterPlugin->GetClusterDataBuffer()->GetAllocatedCount());
        if (data.EnvMap)
            m_StandardResourceSet->BindTexture(7, data.EnvMap->GetBRDFTexture());
        else
            m_StandardResourceSet->BindTextureLayer(7, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        if (data.EnvMap)
        {
            m_StandardResourceSet->BindTexture(8, data.EnvMap->GetPrefilterCubemap());
            m_StandardResourceSet->BindTexture(9, data.EnvMap->GetIrradianceCubemap());
        }
        else
        {
            m_StandardResourceSet->BindTexture(8, m_Renderer->GetDefaultEnvironmentMap());
            m_StandardResourceSet->BindTexture(9, m_Renderer->GetDefaultEnvironmentMap());
        }
        m_StandardResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->WriteTimestamp(0);
        encoder->BindPipeline("standard");
        encoder->BindResourceSet(m_StandardResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 1);
        encoder->FlushResourceSet(1);
        for (auto& pair : meshBatchData.Batches)
        {
            auto& batch = pair.second;
            
            // Draw
            encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            encoder->DrawIndexedIndirect(
                meshBatchData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }

        m_StandardResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_StandardResourceSet->BindBuffer(1, textBatchData.ObjectDataBuffer.get(), 0, textBatchData.ObjectDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(2, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(3, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(4, clusterPlugin->GetLightIndicesBuffer(), 0, clusterPlugin->GetLightIndicesBuffer()->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(5, clusterPlugin->GetLightGridBuffer(), 0, clusterPlugin->GetLightGridBuffer()->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(6, clusterPlugin->GetClusterDataBuffer(), 0, clusterPlugin->GetClusterDataBuffer()->GetAllocatedCount());
        if (data.EnvMap)
            m_StandardResourceSet->BindTexture(7, data.EnvMap->GetBRDFTexture());
        else
            m_StandardResourceSet->BindTextureLayer(7, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        if (data.EnvMap)
        {
            m_StandardResourceSet->BindTexture(8, data.EnvMap->GetPrefilterCubemap());
            m_StandardResourceSet->BindTexture(9, data.EnvMap->GetIrradianceCubemap());
        }
        else
        {
            m_StandardResourceSet->BindTexture(8, m_Renderer->GetDefaultEnvironmentMap());
            m_StandardResourceSet->BindTexture(9, m_Renderer->GetDefaultEnvironmentMap());
        }
        m_StandardResourceSet->FlushBindings();

        encoder->WriteTimestamp(1);
        encoder->BindPipeline("text");
        encoder->BindResourceSet(m_StandardResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 1);
        encoder->FlushResourceSet(1);
        for (auto& batch : textBatchData.Batches)
        {
            m_TextResourceSet->BindTexture(0, batch.FontAtlas);
            m_TextResourceSet->FlushBindings();
            encoder->BindResourceSet(m_TextResourceSet.get(), 2);
            encoder->FlushResourceSet(2);
            
            // Draw
            encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            encoder->DrawIndexedIndirect(
                textBatchData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }
        
        encoder->WriteTimestamp(2);
        encoder->EndEncoding();
    }
}
