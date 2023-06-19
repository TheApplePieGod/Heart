#include "hepch.h"
#include "RenderMeshBatches.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

const auto NORMALS_FORMAT = Flourish::ColorFormat::RGBA16_FLOAT;

namespace Heart::RenderPlugins
{
    void RenderMeshBatches::Initialize()
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            m_Renderer->GetDepthTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::None
        });
        if (m_Info.WriteNormals)
        {
            rpCreateInfo.ColorAttachments.push_back({ NORMALS_FORMAT });
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
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.DepthConfig.CompareOperation = Flourish::DepthComparison::Auto;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void RenderMeshBatches::ResizeInternal()
    {
        if (m_Info.WriteNormals)
        {
            Flourish::TextureCreateInfo texCreateInfo;
            texCreateInfo.Width = m_Renderer->GetRenderWidth();
            texCreateInfo.Height = m_Renderer->GetRenderHeight();
            texCreateInfo.ArrayCount = 1;
            texCreateInfo.MipCount = 1;
            texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
            texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
            texCreateInfo.SamplerState.MinFilter = Flourish::SamplerFilter::Nearest;
            texCreateInfo.SamplerState.MagFilter = Flourish::SamplerFilter::Nearest;
            texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
            texCreateInfo.Format = NORMALS_FORMAT;
            m_NormalsTexture = Flourish::Texture::Create(texCreateInfo);
        }

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        if (m_Info.WriteNormals)
            fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_NormalsTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get());
    }

    void RenderMeshBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderMeshBatches");
        
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto batchesPlugin = m_Renderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        const auto& batchData = batchesPlugin->GetBatchData();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, batchData.ObjectDataBuffer.get(), 0, batchData.ObjectDataBuffer->GetAllocatedCount());
        m_ResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->ClearDepthAttachment();

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
