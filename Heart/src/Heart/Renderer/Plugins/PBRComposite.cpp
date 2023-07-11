#include "hepch.h"
#include "PBRComposite.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/GBuffer.h"
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/ClusteredLighting.h"
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
    void PBRComposite::InitializeInternal()
    {
        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/pbr_composite/Composite.comp", true)->GetShader() };
        m_Pipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = m_Pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void PBRComposite::ResizeInternal()
    {
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddBufferRead(clusterPlugin->GetLightIndicesBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetLightGridBuffer())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer1())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer2())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer3())
            .EncoderAddTextureRead(gBufferPlugin->GetGBufferDepth().get())
            .EncoderAddTextureWrite(m_Renderer->GetRenderTexture().get())
            // Need a read here because we need to ensure current contents are synced
            .EncoderAddTextureRead(m_Renderer->GetRenderTexture().get());
    }

    // TODO: resource sets could definitely be static
    void PBRComposite::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::PBRComposite");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);

        u32 arrayIndex = gBufferPlugin->GetArrayIndex();
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(2, clusterPlugin->GetLightIndicesBuffer(), 0, clusterPlugin->GetLightIndicesBuffer()->GetAllocatedCount());
        m_ResourceSet->BindBuffer(3, clusterPlugin->GetLightGridBuffer(), 0, clusterPlugin->GetLightGridBuffer()->GetAllocatedCount());
        m_ResourceSet->BindBuffer(4, clusterPlugin->GetClusterDataBuffer(), 0, clusterPlugin->GetClusterDataBuffer()->GetAllocatedCount());
        m_ResourceSet->BindTextureLayer(5, gBufferPlugin->GetGBuffer1(), arrayIndex, 0);
        m_ResourceSet->BindTextureLayer(6, gBufferPlugin->GetGBuffer2(), arrayIndex, 0);
        m_ResourceSet->BindTextureLayer(7, gBufferPlugin->GetGBufferDepth(), arrayIndex, 0);
        if (data.EnvMap)
            m_ResourceSet->BindTexture(8, data.EnvMap->GetBRDFTexture());
        else
            m_ResourceSet->BindTextureLayer(8, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        m_ResourceSet->BindTexture(9, m_Renderer->GetRenderTexture().get());

        if (data.EnvMap)
        {
            m_ResourceSet->BindTexture(10, data.EnvMap->GetPrefilterCubemap());
            m_ResourceSet->BindTexture(11, data.EnvMap->GetIrradianceCubemap());
        }
        else
        {
            m_ResourceSet->BindTexture(10, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet->BindTexture(11, m_Renderer->GetDefaultEnvironmentMap());
        }

        m_ResourceSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->Dispatch((m_Renderer->GetRenderWidth() / 16) + 1, (m_Renderer->GetRenderHeight() / 16) + 1, 1);
        encoder->EndEncoding();
    }
}
