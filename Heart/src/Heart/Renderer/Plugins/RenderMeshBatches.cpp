#include "hepch.h"
#include "RenderMeshBatches.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"
#include "Heart/Renderer/SceneRenderer2.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/DescriptorSet.h"

namespace Heart::RenderPlugins
{
    void RenderMeshBatches::Initialize()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Info.Width;
        texCreateInfo.Height = m_Info.Height;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        if (m_Info.WriteNormals)
        {
            texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
            m_NormalsTexture = Flourish::Texture::Create(texCreateInfo);
        }
        texCreateInfo.Format = Flourish::ColorFormat::Depth;
        m_DepthTexture = Flourish::Texture::Create(texCreateInfo);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::Four;
        rpCreateInfo.DepthAttachments.push_back({ Flourish::ColorFormat::Depth });
        if (m_Info.WriteNormals)
        {
            rpCreateInfo.ColorAttachments.push_back({ m_NormalsTexture->GetColorFormat() });
            rpCreateInfo.Subpasses.push_back({
                {},
                { { Flourish::SubpassAttachmentType::Depth, 0 }, { Flourish::SubpassAttachmentType::Color, 0 } }
            });
        }
        else
        {
            rpCreateInfo.Subpasses.push_back({
                {},
                { { Flourish::SubpassAttachmentType::Depth, 0 } }
            });
        }
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        if (m_Info.WriteNormals)
            pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_mesh_batches/Normals.frag", true)->GetShader();
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_mesh_batches/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        pipelineCreateInfo.DepthTest = true;
        pipelineCreateInfo.DepthWrite = true;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Info.Width;
        fbCreateInfo.Height = m_Info.Height;
        if (m_Info.WriteNormals)
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_NormalsTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_DepthTexture });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.MaxEncoders = 1;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::DescriptorSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::DescriptorSetWritability::PerFrame;
        m_DescriptorSet = pipeline->CreateDescriptorSet(0, dsCreateInfo);
    }

    void RenderMeshBatches::Resize(u32 width, u32 height)
    {

    }

    void RenderMeshBatches::RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer)
    {
        HE_PROFILE_FUNCTION();
        
        auto frameDataPlugin = sceneRenderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto batchesPlugin = sceneRenderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        const auto& batchData = batchesPlugin->GetBatchData();

        // TODO: this could probably be static
        m_DescriptorSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_DescriptorSet->BindBuffer(1, batchData.ObjectDataBuffer.get(), 0, batchData.ObjectDataBuffer->GetAllocatedCount());
        m_DescriptorSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindDescriptorSet(m_DescriptorSet.get(), 0);
        encoder->FlushDescriptorSet(0);

        for (auto& pair : batchData.Batches)
        {
            auto& batch = pair.second;
            
            // Draw
            encoder->BindVertexBuffer(batch.Mesh->GetVertexBuffer());
            encoder->BindIndexBuffer(batch.Mesh->GetIndexBuffer());
            encoder->DrawIndexedIndirect(
                batchData.IndirectBuffer.get(), batch.First, batch.Count
            );
        }
        
        encoder->EndEncoding();
    }
}
