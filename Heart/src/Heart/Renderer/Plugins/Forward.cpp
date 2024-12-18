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
#include "Heart/Asset/MeshAsset.h"
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
        auto envMapVert = AssetManager::RetrieveAsset("engine/render_plugins/render_environment_map/Vertex.vert", true);
        auto envMapFrag = AssetManager::RetrieveAsset("engine/render_plugins/render_environment_map/Fragment.frag", true);
        auto postVert = AssetManager::RetrieveAsset("engine/FullscreenTriangle.vert", true);
        auto postFrag = AssetManager::RetrieveAsset("engine/render_plugins/forward/PostProcess.frag", true);
        Asset::LoadMany({
            textFrag, vertShader, standardFrag, envMapVert, envMapFrag,
            postVert, postFrag
        }, false);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            Flourish::ColorFormat::Depth,
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_Info.OutputTexture->GetColorFormat(),
            Flourish::AttachmentInitialization::None,
            (m_Info.OutputTexture->GetUsageType() & Flourish::TextureUsageFlags::Compute) > 0
        });

        // Environment map pass
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
            }
        });

        // Forward pass
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
            }
        });

        // Post processing pass
        rpCreateInfo.Subpasses.push_back({
            {
                { Flourish::SubpassAttachmentType::Color, 0 },
            },
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
        pipelineCreateInfo.CompatibleSubpasses = { 1 };
        auto standardPipeline = m_RenderPass->CreatePipeline("standard", pipelineCreateInfo);
        pipelineCreateInfo.FragmentShader = { textFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        auto textPipeline = m_RenderPass->CreatePipeline("text", pipelineCreateInfo);
        pipelineCreateInfo.VertexShader = { envMapVert->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.FragmentShader = { envMapFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.DepthConfig.DepthTest = false;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CompatibleSubpasses = { 0 };
        auto envMapPipeline = m_RenderPass->CreatePipeline("envmap", pipelineCreateInfo);
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.VertexShader = { postVert->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.FragmentShader = { postFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.CompatibleSubpasses = { 2 };
        auto postProcessPipeline = m_RenderPass->CreatePipeline("post", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        cbCreateInfo.MaxTimestamps = 3; // TODO: disable in dist
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_StandardResourceSet = standardPipeline->CreateResourceSet(0, dsCreateInfo);
        m_TextResourceSet = textPipeline->CreateResourceSet(2, dsCreateInfo);
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_EnvMapResourceSet = envMapPipeline->CreateResourceSet(0, dsCreateInfo);
        m_PostProcessResourceSet = postProcessPipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void Forward::ResizeInternal()
    {
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
            .EncoderAddFramebuffer(m_Framebuffer.get());
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
        const auto& meshBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName)->GetBatchData();
        const auto& textBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeTextBatches>(m_Info.TextBatchesPluginName)->GetBatchData();

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());

        // Environment map pass
        if (data.EnvMap)
        {
            m_EnvMapResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
            m_EnvMapResourceSet->BindTexture(1, data.EnvMap->GetEnvironmentCubemap());
            m_EnvMapResourceSet->FlushBindings();

            encoder->BindPipeline("envmap");
            encoder->BindResourceSet(m_EnvMapResourceSet.get(), 0);
            encoder->FlushResourceSet(0);
            
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
            meshAsset->EnsureValid();
            auto& meshData = meshAsset->GetSubmesh(0);

            encoder->BindVertexBuffer(meshData.GetVertexBuffer());
            encoder->BindIndexBuffer(meshData.GetIndexBuffer());
            encoder->DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                0, 0, 1, 0
            );
        }
        else
            encoder->ClearColorAttachment(0);

        encoder->StartNextSubpass();

        m_StandardResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_StandardResourceSet->BindBuffer(1, meshBatchData.ObjectDataBuffer.get(), 0, meshBatchData.ObjectDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(3, materialBuffer, 0, materialBuffer->GetAllocatedCount());
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
        m_StandardResourceSet->BindBuffer(3, materialBuffer, 0, materialBuffer->GetAllocatedCount());
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

        encoder->StartNextSubpass();

        // Post processing pass

        m_PostProcessResourceSet->BindSubpassInput(0, m_Framebuffer.get(), { Flourish::SubpassAttachmentType::Color, 0 });
        m_PostProcessResourceSet->FlushBindings();
        
        u32 enableTonemap = data.Settings.TonemapEnable;
        encoder->BindPipeline("post");
        encoder->BindResourceSet(m_PostProcessResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(u32), &enableTonemap);
        encoder->Draw(3, 0, 1, 0);
        
        encoder->EndEncoding();
    }
}
