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
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Flourish/Api/ComputePipeline.h"
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
        auto radixShader = AssetManager::RetrieveAsset("engine/render_plugins/splat/RadixSort.comp", true);
        auto histogramShader = AssetManager::RetrieveAsset("engine/render_plugins/splat/RadixSortHistogram.comp", true);
        auto keysShader = AssetManager::RetrieveAsset("engine/render_plugins/splat/ComputeKeys.comp", true);
        Asset::LoadMany({ vertShader, fragShader, radixShader, histogramShader, keysShader }, false);

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
            { true, Flourish::BlendFactor::OneMinusDstAlpha, Flourish::BlendFactor::One, Flourish::BlendFactor::OneMinusDstAlpha, Flourish::BlendFactor::One }
        };
        pipelineCreateInfo.DepthConfig.DepthTest = true;
        pipelineCreateInfo.DepthConfig.DepthWrite = true;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        HE_ENGINE_ASSERT(m_MaxSplats % m_NumBlocksPerWorkgroup == 0);
        u32 maxWorkgroups = (m_MaxSplats / m_NumBlocksPerWorkgroup + m_WorkgroupSize - 1) / m_WorkgroupSize;
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
        bufCreateInfo.Stride = sizeof(u32);
        bufCreateInfo.ElementCount = m_MaxSplats;
        for (u32 i = 0; i < m_SortedKeysBuffers.size(); i++)
            m_SortedKeysBuffers[i] = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        m_KeyBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Type = Flourish::BufferType::Pixel;
        m_CPUBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.ElementCount = maxWorkgroups * m_BinCount;
        m_HistogramBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Stride = sizeof(u32);
        bufCreateInfo.ElementCount = 1;
        m_BuildDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { radixShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_RadixPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { histogramShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_HistogramPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { keysShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_KeysPipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);
        m_RadixResourceSet = m_RadixPipeline->CreateResourceSet(1, dsCreateInfo);
        m_HistorgramResourceSet = m_HistogramPipeline->CreateResourceSet(0, dsCreateInfo);
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_KeysResourceSet = m_KeysPipeline->CreateResourceSet(0, dsCreateInfo);

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

        RebuildGraph(0);
    }

    void Splat::RebuildGraph(u32 splatCount)
    {
        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get());

        for (u32 splat = 0; splat < splatCount; splat++)
        {
            // Radix sort prep
            m_GPUGraphNodeBuilder
                .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddBufferWrite(m_SortedKeysBuffers[0].get())
                .EncoderAddBufferWrite(m_KeyBuffer.get())
                .EncoderAddBufferWrite(m_BuildDataBuffer.get());

            // Radix sort
            for (u32 i = 0; i < 4; i++)
            {
                m_GPUGraphNodeBuilder
                    .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                    .EncoderAddBufferRead(m_SortedKeysBuffers[i % 2].get())
                    .EncoderAddBufferWrite(m_HistogramBuffer.get())
                    .EncoderAddBufferRead(m_KeyBuffer.get())
                    .EncoderAddBufferRead(m_BuildDataBuffer.get())
                    .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                    .EncoderAddBufferRead(m_HistogramBuffer.get())
                    .EncoderAddBufferRead(m_SortedKeysBuffers[i % 2].get())
                    .EncoderAddBufferWrite(m_SortedKeysBuffers[1 - i % 2].get())
                    .EncoderAddBufferRead(m_KeyBuffer.get())
                    .EncoderAddBufferRead(m_BuildDataBuffer.get());
            }

            m_GPUGraphNodeBuilder
                .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
                .EncoderAddBufferRead(m_SortedKeysBuffers[0].get())
                .EncoderAddFramebuffer(m_Framebuffer.get());
        }

        m_GPUGraphNodeBuilder
            .AddEncoderNode(Flourish::GPUWorkloadType::Transfer)
            .EncoderAddBufferRead(m_SortedKeysBuffers[0].get())
            .EncoderAddBufferWrite(m_CPUBuffer.get());
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

            totalSplatInstances++;
            if (totalSplatInstances > m_LastSplatCount)
            {
                // Need to stop early since we have to rebuild the graph
                // before we can render
                continue;
            }

            u32 splatCount = splatAsset->GetDataBuffer()->GetAllocatedCount();
            u32 radixInvocationCount = splatCount / m_NumBlocksPerWorkgroup;
            radixInvocationCount += splatCount % m_NumBlocksPerWorkgroup ? 1 : 0;
            u32 workgroupCount = (radixInvocationCount + m_WorkgroupSize - 1) / m_WorkgroupSize;

            ComputeKeyPushConstants keyPush;
            keyPush.Model = transformData.Transform;
            keyPush.ElemCount = splatCount;
            m_KeysResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
            m_KeysResourceSet->BindBuffer(1, splatAsset->GetDataBuffer(), 0, splatCount);
            m_KeysResourceSet->BindBuffer(2, m_SortedKeysBuffers[0].get(), 0, m_SortedKeysBuffers[0]->GetAllocatedCount());
            m_KeysResourceSet->BindBuffer(3, m_KeyBuffer.get(), 0, m_KeyBuffer->GetAllocatedCount());
            m_KeysResourceSet->BindBuffer(4, m_BuildDataBuffer.get(), 0, 1);
            m_KeysResourceSet->FlushBindings();
            auto compEncoder = m_CommandBuffer->EncodeComputeCommands();
            compEncoder->BindComputePipeline(m_KeysPipeline.get());
            compEncoder->BindResourceSet(m_KeysResourceSet.get(), 0);
            compEncoder->FlushResourceSet(0);
            compEncoder->PushConstants(0, sizeof(ComputeKeyPushConstants), &keyPush);
            compEncoder->Dispatch((splatCount / 256) + 1, 1, 1);
            compEncoder->EndEncoding();

            RadixPushConstants radixPush;
            radixPush.NumWorkgroups = workgroupCount;
            radixPush.NumBlocksPerWorkgroup = m_NumBlocksPerWorkgroup;
            for (u32 i = 0; i < 4; i++)
            {
                radixPush.Shift = 8 * i;

                m_HistorgramResourceSet->BindBuffer(
                    0, m_SortedKeysBuffers[i % 2].get(), 0, m_SortedKeysBuffers[0]->GetAllocatedCount()
                );
                m_HistorgramResourceSet->BindBuffer(
                    1, m_HistogramBuffer.get(), 0, m_HistogramBuffer->GetAllocatedCount()
                );
                m_HistorgramResourceSet->BindBuffer(2, m_KeyBuffer.get(), 0, m_KeyBuffer->GetAllocatedCount());
                m_HistorgramResourceSet->BindBuffer(3, m_BuildDataBuffer.get(), 0, 1);
                m_HistorgramResourceSet->FlushBindings();
                auto compEncoder = m_CommandBuffer->EncodeComputeCommands();
                compEncoder->BindComputePipeline(m_HistogramPipeline.get());
                compEncoder->BindResourceSet(m_HistorgramResourceSet.get(), 0);
                compEncoder->FlushResourceSet(0);
                compEncoder->PushConstants(0, sizeof(RadixPushConstants), &radixPush);
                compEncoder->Dispatch(workgroupCount, 1, 1);
                compEncoder->EndEncoding();

                m_RadixResourceSet->BindBuffer(
                    i % 2, m_SortedKeysBuffers[0].get(), 0, m_SortedKeysBuffers[0]->GetAllocatedCount()
                );
                m_RadixResourceSet->BindBuffer(
                    1 - (i % 2), m_SortedKeysBuffers[1].get(), 0, m_SortedKeysBuffers[1]->GetAllocatedCount()
                );
                m_RadixResourceSet->BindBuffer(
                    2, m_HistogramBuffer.get(), 0, m_HistogramBuffer->GetAllocatedCount()
                );
                m_RadixResourceSet->BindBuffer(3, m_KeyBuffer.get(), 0, m_KeyBuffer->GetAllocatedCount());
                m_RadixResourceSet->BindBuffer(4, m_BuildDataBuffer.get(), 0, 1);
                m_RadixResourceSet->FlushBindings();
                compEncoder = m_CommandBuffer->EncodeComputeCommands();
                compEncoder->BindComputePipeline(m_RadixPipeline.get());
                compEncoder->BindResourceSet(m_RadixResourceSet.get(), 1);
                compEncoder->FlushResourceSet(1);
                compEncoder->PushConstants(0, sizeof(RadixPushConstants), &radixPush);
                compEncoder->Dispatch(workgroupCount, 1, 1);
                compEncoder->EndEncoding();
            }

            m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
            m_ResourceSet->BindBuffer(1, splatAsset->GetDataBuffer(), 0, splatCount);
            m_ResourceSet->BindBuffer(
                2, m_SortedKeysBuffers[0].get(), 0, m_SortedKeysBuffers[0]->GetAllocatedCount()
            );
            m_ResourceSet->BindBuffer(3, m_BuildDataBuffer.get(), 0, 1);
            m_ResourceSet->FlushBindings();

            auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
            encoder->BindPipeline("main");
            encoder->BindResourceSet(m_ResourceSet.get(), 0);
            encoder->FlushResourceSet(0);
            encoder->BindVertexBuffer(meshData.GetVertexBuffer());
            encoder->BindIndexBuffer(meshData.GetIndexBuffer());

            encoder->PushConstants(0, sizeof(glm::mat4), &transformData.Transform);
            encoder->DrawIndexed(
                6, // Render once face
                12, 0, splatCount, 0
            );
            encoder->EndEncoding();

            totalSplatCount += splatCount;
        }

        auto tEncoder = m_CommandBuffer->EncodeTransferCommands();
        tEncoder->CopyBufferToBuffer(
            m_SortedKeysBuffers[0].get(),
            m_CPUBuffer.get(),
            0, 0, 50
        );
        tEncoder->FlushBuffer(m_CPUBuffer.get());
        tEncoder->EndEncoding();

        u32 elems[50];
        m_CPUBuffer->ReadBytes(elems, sizeof(elems), 0);
        HE_LOG_WARN(
            "{0}, {1}, {2}, {3}, {4}",
            elems[0],
            elems[1],
            elems[2],
            elems[3],
            elems[4]
        );

        if (totalSplatInstances > m_LastSplatCount)
        {
            RebuildGraph(totalSplatInstances);
            m_Renderer->QueueGraphRebuild();
        }

        m_LastSplatCount = totalSplatInstances;
        m_Stats["Instance Count"] = {
            StatType::Int,
            (int)totalSplatInstances
        };
        m_Stats["Splat Count"] = {
            StatType::Int,
            (int)totalSplatCount
        };
    }
}
