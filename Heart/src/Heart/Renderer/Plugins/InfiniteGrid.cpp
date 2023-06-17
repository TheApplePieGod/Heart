#include "hepch.h"
#include "InfiniteGrid.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/TransparencyComposite.h"
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
    void InfiniteGrid::Initialize()
    {
        auto tpPlugin = m_Renderer->GetPlugin<RenderPlugins::TransparencyComposite>(m_Info.TransparencyCompositePluginName);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            m_Renderer->GetDepthTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.ColorAttachments.push_back({
            tpPlugin->GetAccumTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.ColorAttachments.push_back({
            tpPlugin->GetRevealTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 },
                { Flourish::SubpassAttachmentType::Color, 1 }
            }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/infinite_grid/Fragment.frag", true)->GetShader();
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/infinite_grid/Vertex.vert", true)->GetShader();
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.BlendStates = {
            { true, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One, Flourish::BlendFactor::One },
            { true, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcColor, Flourish::BlendFactor::Zero, Flourish::BlendFactor::OneMinusSrcAlpha }
        };
        // Test must also be true in order for gl_FragDepth to work
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, tpPlugin->GetAccumTexture() });
        fbCreateInfo.ColorAttachments.push_back({ { 1.f }, tpPlugin->GetRevealTexture() });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
    }

    void InfiniteGrid::ResizeInternal()
    {

    }

    void InfiniteGrid::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::InfiniteGrid");

        // TODO: maybe we want to make the plugins dyanmic rather than rely on config inside
        // individual plugins themselves
        //if (!data.Settings.DrawGrid)
        //    return;
        
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);

        encoder->Draw(6, 0, 1, 0);

        encoder->EndEncoding();
    }
}
