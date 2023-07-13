#include "hepch.h"
#include "GBuffer.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"
#include "Heart/Renderer/Plugins/ComputeTextBatches.h"
#include "Heart/Renderer/Plugins/EntityIds.h"
#include "Heart/Renderer/Plugins/CollectMaterials.h"
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

const auto GBUFFER_FORMAT = Flourish::ColorFormat::RGBA16_FLOAT;

namespace Heart::RenderPlugins
{
    u32 GBuffer::GetArrayIndex() const
    {
        return Flourish::Context::FrameCount() % 2;
    }

    u32 GBuffer::GetPrevArrayIndex() const
    {
        return (Flourish::Context::FrameCount() - 1) % 2;
    }

    void GBuffer::InitializeInternal()
    {
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        // Queue shader loads 
        AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Vertex.vert", true, true, true);
        AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Standard.frag", true, true, true);
        AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Text.frag", true, true, true);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            Flourish::ColorFormat::Depth,
            Flourish::AttachmentInitialization::None // Cleared manually
        });
        rpCreateInfo.ColorAttachments.push_back({
            GBUFFER_FORMAT,
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.ColorAttachments.push_back({
            GBUFFER_FORMAT,
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.ColorAttachments.push_back({
            GBUFFER_FORMAT,
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
        if (eidPlugin)
        {
            rpCreateInfo.ColorAttachments.push_back({ eidPlugin->GetTexture()->GetColorFormat() });
            rpCreateInfo.Subpasses.back().OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 3 });
        }

        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = { AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Standard.frag", true)->GetShader() };
        pipelineCreateInfo.VertexShader = { AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Vertex.vert", true)->GetShader() };
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false }, { false }, { false } };
        if (eidPlugin)
            pipelineCreateInfo.BlendStates.push_back({ false });
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        pipelineCreateInfo.AccessOverrides = {
            { 1, 0, Flourish::ShaderTypeFlags::All }
        };
        auto standardPipeline = m_RenderPass->CreatePipeline("standard", pipelineCreateInfo);
        pipelineCreateInfo.FragmentShader = { AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/gbuffer/Text.frag", true)->GetShader() };
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        auto textPipeline = m_RenderPass->CreatePipeline("text", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_StandardResourceSet = standardPipeline->CreateResourceSet(0, dsCreateInfo);
        m_TextResourceSet = textPipeline->CreateResourceSet(2, dsCreateInfo);

        ResizeInternal();
    }

    void GBuffer::ResizeInternal()
    {
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        // TODO: need to revisit writability
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 2;
        texCreateInfo.MipCount = 2;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        texCreateInfo.Writability = Flourish::TextureWritability::Once;
        texCreateInfo.SamplerState.MinFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.MagFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = GBUFFER_FORMAT;
        m_GBuffer1 = Flourish::Texture::Create(texCreateInfo);
        m_GBuffer2 = Flourish::Texture::Create(texCreateInfo);
        m_GBuffer3 = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        m_GBufferDepth = Flourish::Texture::Create(texCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        for (u32 i = 0; i < 2; i++)
        {
            fbCreateInfo.ColorAttachments.clear();
            fbCreateInfo.DepthAttachments.clear();
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_GBuffer1, i });
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_GBuffer2, i });
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, -1.f, 0.f }, m_GBuffer3, i });
            if (eidPlugin)
                fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, eidPlugin->GetTexture() });
            fbCreateInfo.DepthAttachments.push_back({ m_GBufferDepth, i });
            m_Framebuffers[i] = Flourish::Framebuffer::Create(fbCreateInfo);
        }

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffers[0].get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddTextureRead(m_GBuffer1.get())
            .EncoderAddTextureRead(m_GBuffer2.get())
            .EncoderAddTextureRead(m_GBuffer3.get())
            .EncoderAddTextureRead(m_GBufferDepth.get())
            .EncoderAddTextureWrite(m_GBuffer1.get())
            .EncoderAddTextureWrite(m_GBuffer2.get())
            .EncoderAddTextureWrite(m_GBuffer3.get())
            .EncoderAddTextureWrite(m_GBufferDepth.get())
            .EncoderAddTextureWrite(m_Renderer->GetDepthTexture().get());
    }

    void GBuffer::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::GBuffer");
        
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        auto materialBuffer = materialsPlugin->GetMaterialBuffer();
        const auto& meshBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName)->GetBatchData();
        const auto& textBatchData = m_Renderer->GetPlugin<RenderPlugins::ComputeTextBatches>(m_Info.TextBatchesPluginName)->GetBatchData();

        m_StandardResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_StandardResourceSet->BindBuffer(1, meshBatchData.ObjectDataBuffer.get(), 0, meshBatchData.ObjectDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(2, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_StandardResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffers[GetArrayIndex()].get());
        encoder->BindPipeline("standard");
        encoder->BindResourceSet(m_StandardResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 1);
        encoder->FlushResourceSet(1);
        encoder->ClearDepthAttachment();
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
        m_StandardResourceSet->BindBuffer(2, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_StandardResourceSet->FlushBindings();

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
        
        encoder->EndEncoding();

        auto graphicsEncoder = m_CommandBuffer->EncodeGraphicsCommands();

        // Generate mips for gbuffer
        graphicsEncoder->GenerateMipMaps(m_GBuffer1.get(), Flourish::SamplerFilter::Nearest);
        graphicsEncoder->GenerateMipMaps(m_GBuffer2.get(), Flourish::SamplerFilter::Nearest);
        graphicsEncoder->GenerateMipMaps(m_GBuffer3.get(), Flourish::SamplerFilter::Nearest);
        graphicsEncoder->GenerateMipMaps(m_GBufferDepth.get(), Flourish::SamplerFilter::Nearest);

        // Blit depth to render depth for ease of use
        graphicsEncoder->BlitTexture(
            m_GBufferDepth.get(),
            m_Renderer->GetDepthTexture().get(),
            GetArrayIndex(), 0,
            0, 0
        );
        
        graphicsEncoder->EndEncoding();
    }
}
