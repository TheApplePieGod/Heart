#include "hepch.h"
#include "ClusteredLighting.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/Context.h"

namespace Heart::RenderPlugins
{
    void ClusteredLighting::InitializeInternal()
    {
        // Queue shader loads 
        auto buildAsset = AssetManager::RetrieveAsset("engine/render_plugins/clustered_lighting/Build.comp", true);
        auto cullAsset = AssetManager::RetrieveAsset("engine/render_plugins/clustered_lighting/Cull.comp", true);
        Asset::LoadMany({ buildAsset, cullAsset }, false);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { buildAsset->EnsureValid<ShaderAsset>()->GetShader() };
        m_BuildPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { cullAsset->EnsureValid<ShaderAsset>()->GetShader() };
        m_CullPipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        m_BuildResourceSet = m_BuildPipeline->CreateResourceSet(0, dsCreateInfo);
        m_CullResourceSet = m_CullPipeline->CreateResourceSet(0, dsCreateInfo);

        u32 xClusters = 16;
        u32 yClusters = 8;
        u32 zClusters = 24;
        u32 maxLightsPerCluster = 100;
        u32 clusterCount = xClusters * yClusters * zClusters;
        m_PushData.ClusterDims = { xClusters, yClusters, zClusters, 0 };

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::GPUOnly;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Storage;
        bufCreateInfo.Stride = sizeof(Cluster);
        bufCreateInfo.ElementCount = clusterCount;
        m_ClusterBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Stride = sizeof(glm::vec4);
        m_LightGridBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.ElementCount = clusterCount * maxLightsPerCluster / 4;
        m_LightIndicesBuffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.ElementCount = 1;
        m_BuildData = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPUWriteFrame;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Uniform;
        bufCreateInfo.Stride = sizeof(ClusterData);
        m_ClusterData = Flourish::Buffer::Create(bufCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddBufferWrite(m_ClusterBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddBufferRead(m_ClusterBuffer.get())
            .EncoderAddBufferWrite(m_LightGridBuffer.get())
            .EncoderAddBufferWrite(m_LightIndicesBuffer.get());

        ResizeInternal();
    }

    void ClusteredLighting::ResizeInternal()
    {

    }

    void ClusteredLighting::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ClusteredLighting");

        // Update scale and bias based on camera info
        ClusterData clusterData;
        float logClips = log(data.Camera->GetFarClip() / data.Camera->GetNearClip()); 
        clusterData.ClusterScale = m_PushData.ClusterDims.z / logClips;
        clusterData.ClusterBias = -(m_PushData.ClusterDims.z * log(data.Camera->GetNearClip()) / logClips);
        clusterData.ClusterDims = m_PushData.ClusterDims;
        m_ClusterData->SetElements(&clusterData, 1, 0);

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();

        // TODO: only need to rebuild if camera or screen dims change
        m_BuildResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_BuildResourceSet->BindBuffer(1, m_ClusterBuffer.get(), 0, m_ClusterBuffer->GetAllocatedCount());
        m_BuildResourceSet->FlushBindings();

        m_CullResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_CullResourceSet->BindBuffer(1, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_CullResourceSet->BindBuffer(2, m_ClusterBuffer.get(), 0, m_ClusterBuffer->GetAllocatedCount());
        m_CullResourceSet->BindBuffer(3, m_LightIndicesBuffer.get(), 0, m_LightIndicesBuffer->GetAllocatedCount());
        m_CullResourceSet->BindBuffer(4, m_LightGridBuffer.get(), 0, m_LightGridBuffer->GetAllocatedCount());
        m_CullResourceSet->BindBuffer(5, m_BuildData.get(), 0, 1);
        m_CullResourceSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_BuildPipeline.get());
        encoder->BindResourceSet(m_BuildResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(PushData), &m_PushData);
        encoder->Dispatch(
            m_PushData.ClusterDims.x / 16,
            m_PushData.ClusterDims.y / 8,
            m_PushData.ClusterDims.z / 4
        );
        encoder->EndEncoding();

        encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_CullPipeline.get());
        encoder->BindResourceSet(m_CullResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->Dispatch(
            m_PushData.ClusterDims.x / 16,
            m_PushData.ClusterDims.y / 8,
            m_PushData.ClusterDims.z / 4
        );
        encoder->EndEncoding();
    }
}
