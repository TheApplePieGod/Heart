#include "hepch.h"
#include "Splat.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/SplatAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void Splat::InitializeInternal()
    {
        // Queue shader loads 
        auto vertShader = AssetManager::RetrieveAsset("engine/render_plugins/splat/Splat.vert", true);
        auto fragShader = AssetManager::RetrieveAsset("engine/render_plugins/splat/Splat.frag", true);
        Asset::LoadMany({ vertShader, fragShader }, false);

        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.DepthAttachments.push_back({
            m_Renderer->GetDepthTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.ColorAttachments.push_back({
            m_Renderer->GetRenderTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve,
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
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::TriangleList;
        pipelineCreateInfo.VertexLayout = Mesh::GetVertexLayout();
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = {
            { true, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::OneMinusSrcAlpha, Flourish::BlendFactor::Zero, Flourish::BlendFactor::One }
        };
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void Splat::ResizeInternal()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        fbCreateInfo.DepthAttachments.push_back({ m_Renderer->GetDepthTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get());
    }

    void Splat::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::Splat");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        
        auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>("engine/DefaultCube.gltf", true);
        meshAsset->EnsureValid();
        auto& meshData = meshAsset->GetSubmesh(0);

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindVertexBuffer(meshData.GetVertexBuffer());
        encoder->BindIndexBuffer(meshData.GetIndexBuffer());

        u32 totalSplatInstances = 0;
        u32 totalSplatCount = 0;
        auto splatView = data.Scene->GetRegistry().view<SplatComponent>();
        for (entt::entity entity : splatView)
        {
            const auto& splatComp = splatView.get<SplatComponent>(entity);
            const auto& transformData = data.Scene->GetCachedTransforms().at(entity);
            auto splatAsset = AssetManager::RetrieveAsset<SplatAsset>(splatComp.Splat);
            if (!splatAsset || !splatAsset->Load(!data.Settings.AsyncAssetLoading)->IsValid())
                continue;

            u32 splatCount = splatAsset->GetTransformBuffer()->GetAllocatedCount();
            m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
            m_ResourceSet->BindBuffer(1, splatAsset->GetTransformBuffer(), 0, splatCount);
            m_ResourceSet->BindBuffer(2, splatAsset->GetColorBuffer(), 0, splatCount);
            m_ResourceSet->FlushBindings();

            encoder->PushConstants(0, sizeof(glm::mat4), &transformData.Transform);
            encoder->DrawIndexed(
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                0, 0, splatCount, 0
            );

            totalSplatInstances++;
            totalSplatCount += splatCount;
        }

        m_Stats["Instance Count"] = {
            StatType::Int,
            (int)totalSplatInstances
        };
        m_Stats["Splat Count"] = {
            StatType::Int,
            (int)totalSplatCount
        };

        encoder->EndEncoding();
    }
}
