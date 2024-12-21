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

namespace Heart::RenderPlugins
{
    GBuffer::GBuffer(SceneRenderer* renderer, HStringView8 name, const GBufferCreateInfo& createInfo)
        : RenderPlugin(renderer, name), m_Info(createInfo)
    {
        m_NormalData = Flourish::Texture::CreatePlaceholder(
            m_Info.StoreMotionVectors ? Flourish::ColorFormat::RGBA16_FLOAT : Flourish::ColorFormat::RG16_FLOAT
        );
        m_ColorData = Flourish::Texture::CreatePlaceholder(Flourish::ColorFormat::RGBA8_UINT);
        m_EmissiveData = Flourish::Texture::CreatePlaceholder(Flourish::ColorFormat::RGBA8_UNORM);
        m_Depth = Flourish::Texture::CreatePlaceholder(Flourish::ColorFormat::Depth);
    }

    u32 GBuffer::GetArrayIndex() const
    {
        return Flourish::Context::FrameCount() % m_ImageCount;
    }

    u32 GBuffer::GetPrevArrayIndex() const
    {
        return (Flourish::Context::FrameCount() - 1) % m_ImageCount;
    }

    void GBuffer::InitializeInternal()
    {
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        m_ImageCount = m_Info.KeepHistory ? 2 : 1;

        // Queue shader loads 
        auto textFrag = AssetManager::RetrieveAsset("engine/render_plugins/gbuffer/Text.frag", true);
        auto vertShader = AssetManager::RetrieveAsset("engine/render_plugins/gbuffer/Vertex.vert", true);
        auto standardFrag = AssetManager::RetrieveAsset("engine/render_plugins/gbuffer/Standard.frag", true);
        Asset::LoadMany({ textFrag, vertShader, standardFrag }, false);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            Flourish::ColorFormat::Depth,
            Flourish::AttachmentInitialization::None // Cleared manually
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_NormalData->GetColorFormat(),
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
            }
        });
        if (m_Info.StoreColorAndEmissiveData)
        {
            rpCreateInfo.ColorAttachments.push_back({
                m_ColorData->GetColorFormat(),
                Flourish::AttachmentInitialization::Clear
            });
            rpCreateInfo.ColorAttachments.push_back({
                m_EmissiveData->GetColorFormat(),
                Flourish::AttachmentInitialization::Clear
            });
            rpCreateInfo.Subpasses[0].OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 1 });
            rpCreateInfo.Subpasses[0].OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 2 });
        }
        if (eidPlugin)
        {
            rpCreateInfo.ColorAttachments.push_back({ eidPlugin->GetTexture()->GetColorFormat() });
            rpCreateInfo.Subpasses.back().OutputAttachments.push_back({ Flourish::SubpassAttachmentType::Color, 3 });
        }

        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = { standardFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.VertexShader = { vertShader->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        for (u32 i = 1; i < rpCreateInfo.Subpasses[0].OutputAttachments.size(); i++)
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
        pipelineCreateInfo.FragmentShader = { textFrag->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        auto textPipeline = m_RenderPass->CreatePipeline("text", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        cbCreateInfo.MaxTimestamps = 3; // TODO: disable in dist
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        m_StandardResourceSet = standardPipeline->CreateResourceSet(0, dsCreateInfo);
        m_TextResourceSet = textPipeline->CreateResourceSet(2, dsCreateInfo);

        ResizeInternal();
    }

    void GBuffer::ResizeInternal()
    {
        auto eidPlugin = m_Renderer->GetPlugin<RenderPlugins::EntityIds>(m_Info.EntityIdsPluginName);

        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.MipCount = m_Info.MipCount;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        texCreateInfo.SamplerState.MinFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.MagFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.ArrayCount = m_ImageCount;
        texCreateInfo.Format = m_NormalData->GetColorFormat();
        Flourish::Texture::Replace(m_NormalData, texCreateInfo);
        texCreateInfo.ArrayCount = m_ImageCount;
        texCreateInfo.Format = m_Depth->GetColorFormat();
        Flourish::Texture::Replace(m_Depth, texCreateInfo);
        if (m_Info.StoreColorAndEmissiveData)
        {
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.Format = m_ColorData->GetColorFormat();
            Flourish::Texture::Replace(m_ColorData, texCreateInfo);
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.Format = m_EmissiveData->GetColorFormat();
            Flourish::Texture::Replace(m_EmissiveData, texCreateInfo);
        }

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        for (u32 i = 0; i < m_ImageCount; i++)
        {
            fbCreateInfo.ColorAttachments.clear();
            fbCreateInfo.DepthAttachments.clear();
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_NormalData, i });
            if (m_Info.StoreColorAndEmissiveData)
            {
                fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_ColorData, 0 });
                fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_EmissiveData, 0 });
            }
            if (eidPlugin)
                fbCreateInfo.ColorAttachments.push_back({ { -1.f, 0.f, 0.f, 0.f }, eidPlugin->GetTexture() });
            fbCreateInfo.DepthAttachments.push_back({ m_Depth, i });
            m_Framebuffers[i] = Flourish::Framebuffer::Create(fbCreateInfo);
        }

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffers[0].get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddTextureRead(m_NormalData.get())
            .EncoderAddTextureRead(m_ColorData.get())
            .EncoderAddTextureRead(m_EmissiveData.get())
            .EncoderAddTextureRead(m_Depth.get())
            .EncoderAddTextureWrite(m_NormalData.get())
            .EncoderAddTextureWrite(m_ColorData.get())
            .EncoderAddTextureWrite(m_EmissiveData.get())
            .EncoderAddTextureWrite(m_Depth.get());

        m_DebugTextures.clear();
        if (m_Renderer->IsDebug())
        {
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics;
            texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
            m_DebugTextures["Albedo"] = Flourish::Texture::Create(texCreateInfo);
            m_DebugTextures["Normal"] = Flourish::Texture::Create(texCreateInfo);
            m_DebugTextures["Motion Vector"] = Flourish::Texture::Create(texCreateInfo);
            m_DebugTextures["Metallic Roughness"] = Flourish::Texture::Create(texCreateInfo);
            m_DebugTextures["Depth"] = m_Depth;
        }
    }

    void GBuffer::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::GBuffer");

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

        m_StandardResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_StandardResourceSet->BindBuffer(1, meshBatchData.ObjectDataBuffer.get(), 0, meshBatchData.ObjectDataBuffer->GetAllocatedCount());
        m_StandardResourceSet->BindBuffer(2, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_StandardResourceSet->FlushBindings();

        m_PushData.StoreMotionVectors = m_Info.StoreMotionVectors;
        m_PushData.StoreColorAndEmissive = m_Info.StoreColorAndEmissiveData;
        m_PushData.StoreEntityIds = !m_Info.EntityIdsPluginName.IsEmpty();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffers[GetArrayIndex()].get());
        encoder->WriteTimestamp(0);
        encoder->BindPipeline("standard");
        encoder->BindResourceSet(m_StandardResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 1);
        encoder->FlushResourceSet(1);
        encoder->ClearDepthAttachment();
        encoder->PushConstants(0, sizeof(PushData), &m_PushData);
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

        encoder->WriteTimestamp(1);
        encoder->BindPipeline("text");
        encoder->BindResourceSet(m_StandardResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 1);
        encoder->FlushResourceSet(1);
        encoder->PushConstants(0, sizeof(PushData), &m_PushData);
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

        auto graphicsEncoder = m_CommandBuffer->EncodeGraphicsCommands();

        if (m_Info.MipCount != 1)
        {
            // TODO: should be able target current layer
            graphicsEncoder->GenerateMipMaps(m_NormalData.get(), Flourish::SamplerFilter::Nearest);
            if (m_Info.StoreColorAndEmissiveData)
            {
                graphicsEncoder->GenerateMipMaps(m_ColorData.get(), Flourish::SamplerFilter::Nearest);
                graphicsEncoder->GenerateMipMaps(m_EmissiveData.get(), Flourish::SamplerFilter::Nearest);
            }
            graphicsEncoder->GenerateMipMaps(m_Depth.get(), Flourish::SamplerFilter::Nearest);
        }
        
        graphicsEncoder->EndEncoding();
    }
}
