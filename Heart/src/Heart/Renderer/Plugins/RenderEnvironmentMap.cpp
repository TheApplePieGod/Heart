#include "hepch.h"
#include "RenderEnvironmentMap.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void RenderEnvironmentMap::Initialize()
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.ColorAttachments.push_back({
            m_Renderer->GetRenderTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            { { Flourish::SubpassAttachmentType::Color, 0 } }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_environment_map/Fragment.frag", true)->GetShader();
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/render_environment_map/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        pipelineCreateInfo.DepthConfig.DepthTest = false;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
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

    void RenderEnvironmentMap::ResizeInternal()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get());
    }

    void RenderEnvironmentMap::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RenderEnvironmentMap");

        // TODO: maybe we want to make the plugins dyanmic rather than rely on config inside
        // individual plugins themselves
        if (!data.EnvMap)
            return;
        
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindTexture(1, data.EnvMap->GetEnvironmentCubemap());
        m_ResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);

        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
        auto& meshData = meshAsset->GetSubmesh(0);

        encoder->BindVertexBuffer(meshData.GetVertexBuffer());
        encoder->BindIndexBuffer(meshData.GetIndexBuffer());
        encoder->DrawIndexed(
            meshData.GetIndexBuffer()->GetAllocatedCount(),
            0, 0, 1, 0
        );

        encoder->EndEncoding();
    }
}
