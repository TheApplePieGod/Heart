#include "hepch.h"
#include "InfiniteGrid.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void InfiniteGrid::InitializeInternal()
    {
        // Queue shader loads 
        auto vertShader = AssetManager::RetrieveAsset("engine/render_plugins/infinite_grid/Vertex.vert", true);
        auto fragShader = AssetManager::RetrieveAsset("engine/render_plugins/infinite_grid/Fragment.frag", true);
        Asset::LoadMany({ vertShader, fragShader }, false);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            m_Info.OutputDepthTexture->GetColorFormat(),
            m_Info.ClearDepthOutput ? Flourish::AttachmentInitialization::Clear : Flourish::AttachmentInitialization::Preserve,
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_Info.OutputColorTexture->GetColorFormat(),
            m_Info.ClearColorOutput ? Flourish::AttachmentInitialization::Clear : Flourish::AttachmentInitialization::Preserve,
            true
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            {
                { Flourish::SubpassAttachmentType::Depth, 0 },
                { Flourish::SubpassAttachmentType::Color, 0 }
            }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = { vertShader->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.FragmentShader = { fragShader->EnsureValid<ShaderAsset>()->GetShader() };
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.BlendStates = {
            { true, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::OneMinusSrcAlpha, Flourish::BlendFactor::Zero, Flourish::BlendFactor::One }
        };
        // Test must also be true in order for gl_FragDepth to work
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        cbCreateInfo.MaxTimestamps = 2;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void InfiniteGrid::ResizeInternal()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Info.OutputColorTexture->GetWidth();
        fbCreateInfo.Height = m_Info.OutputColorTexture->GetHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Info.OutputColorTexture });
        fbCreateInfo.DepthAttachments.push_back({ m_Info.OutputDepthTexture });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get());
    }

    void InfiniteGrid::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::InfiniteGrid");

        m_Stats["GPU Time"].Type = StatType::TimeMS;
        m_Stats["GPU Time"].Data.Float = (float)(m_CommandBuffer->ComputeTimestampDifference(0, 1) * 1e-6);

        if (!data.Settings.DrawGrid)
        {
            m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get())->EndEncoding();
            return;
        }

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->WriteTimestamp(0);
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->Draw(6, 0, 1, 0);
        encoder->WriteTimestamp(1);
        encoder->EndEncoding();
    }
}
