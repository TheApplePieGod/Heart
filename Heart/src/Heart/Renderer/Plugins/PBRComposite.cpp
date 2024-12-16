#include "hepch.h"
#include "PBRComposite.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/SSAO.h"
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
        auto shader = AssetManager::RetrieveAsset("engine/render_plugins/pbr_composite/Composite.comp", true);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { shader->EnsureValid<ShaderAsset>()->GetShader() };
        m_Pipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        cbCreateInfo.MaxTimestamps = 2;
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = m_Pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void PBRComposite::ResizeInternal()
    {
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddBufferRead(clusterPlugin->GetLightIndicesBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetLightGridBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetClusterDataBuffer())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer1().get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer2().get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer3().get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBufferDepth().get())
            .EncoderAddTextureRead(ssaoPlugin->GetOutputTexture())
            .EncoderAddTextureWrite(m_Info.OutputTexture.get());
    }

    // TODO: resource sets could definitely be static
    void PBRComposite::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::PBRComposite");

        m_Stats["GPU Time"].Type = StatType::TimeMS;
        m_Stats["GPU Time"].Data.Float = (float)(m_CommandBuffer->ComputeTimestampDifference(0, 1) * 1e-6);

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);

        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet->BindBuffer(2, clusterPlugin->GetLightIndicesBuffer(), 0, clusterPlugin->GetLightIndicesBuffer()->GetAllocatedCount());
        m_ResourceSet->BindBuffer(3, clusterPlugin->GetLightGridBuffer(), 0, clusterPlugin->GetLightGridBuffer()->GetAllocatedCount());
        m_ResourceSet->BindBuffer(4, clusterPlugin->GetClusterDataBuffer(), 0, clusterPlugin->GetClusterDataBuffer()->GetAllocatedCount());
        m_ResourceSet->BindTextureLayer(5, gBufferPlugin->GetGBuffer1(), 0, 0);
        m_ResourceSet->BindTextureLayer(6, gBufferPlugin->GetGBuffer2(), 0, 0);
        m_ResourceSet->BindTextureLayer(7, gBufferPlugin->GetGBufferDepth(), 0, 0);
        if (data.EnvMap)
            m_ResourceSet->BindTexture(8, data.EnvMap->GetBRDFTexture());
        else
            m_ResourceSet->BindTextureLayer(8, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        m_ResourceSet->BindTexture(9, m_Info.OutputTexture.get());

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
        m_ResourceSet->BindTexture(12, ssaoPlugin->GetOutputTexture());

        m_ResourceSet->FlushBindings();

        m_PushConstants.SSAOEnable = data.Settings.SSAOEnable;

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->WriteTimestamp(0);
        encoder->BindComputePipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(PushConstants), &m_PushConstants);
        encoder->Dispatch((m_Info.OutputTexture->GetWidth() / 16) + 1, (m_Info.OutputTexture->GetHeight() / 16) + 1, 1);
        encoder->WriteTimestamp(1);
        encoder->EndEncoding();
    }
}
