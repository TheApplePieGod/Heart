#include "hepch.h"
#include "RayPBRComposite.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/GBuffer.h"
#include "Heart/Renderer/Plugins/SSAO.h"
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/RayTracing/TLAS.h"
#include "Heart/Renderer/Plugins/ClusteredLighting.h"
#include "Heart/Renderer/Plugins/CollectMaterials.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/RayTracing/RayTracingPipeline.h"
#include "Flourish/Api/RayTracing/RayTracingGroupTable.h"

namespace Heart::RenderPlugins
{
    void RayPBRComposite::InitializeInternal()
    {
        // Queue shader loads 
        auto raygen = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_pbr_composite/RayGen.rgen", true);
        auto anyhit = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_pbr_composite/Shadow.rahit", true);
        auto miss = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_pbr_composite/Shadow.rmiss", true);
        Asset::LoadMany({ raygen, anyhit, miss }, false);

        Flourish::RayTracingPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.MaxRayRecursionDepth = 1;
        pipelineCreateInfo.Shaders = {
            { raygen->EnsureValid<ShaderAsset>()->GetShader() },
            { anyhit->EnsureValid<ShaderAsset>()->GetShader() },
            { miss->EnsureValid<ShaderAsset>()->GetShader() },
        };
        pipelineCreateInfo.AccessOverrides = {
            { 2, 0, Flourish::ShaderTypeFlags::All }
        };
        
        // RayGen
        {
            Flourish::RayTracingShaderGroup group;
            group.GeneralShader = 0;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        // Shadow hit
        {
            Flourish::RayTracingShaderGroup group;
            group.AnyHitShader = 1;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        // Shadow miss
        {
            Flourish::RayTracingShaderGroup group;
            group.GeneralShader = 2;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        m_Pipeline = Flourish::RayTracingPipeline::Create(pipelineCreateInfo);

        Flourish::RayTracingGroupTableCreateInfo gtCreateInfo;
        gtCreateInfo.Pipeline = m_Pipeline;
        gtCreateInfo.MaxHitEntries = 1;
        m_GroupTable = Flourish::RayTracingGroupTable::Create(gtCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet0 = m_Pipeline->CreateResourceSet(0, dsCreateInfo);
        m_ResourceSet1 = m_Pipeline->CreateResourceSet(1, dsCreateInfo);

        ResizeInternal();
    }

    void RayPBRComposite::ResizeInternal()
    {
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);
        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddBufferRead(clusterPlugin->GetLightIndicesBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetLightGridBuffer())
            .EncoderAddBufferRead(clusterPlugin->GetClusterDataBuffer())
            .EncoderAddTextureRead(gBufferPlugin->GetNormalData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetColorData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetEmissiveData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetDepth().get())
            .EncoderAddTextureRead(ssaoPlugin->GetOutputTexture())
            .EncoderAddTextureRead(m_Info.ReflectionsInputTexture.get())
            .EncoderAddTextureWrite(m_Info.OutputTexture.get());
    }

    void RayPBRComposite::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RayPBRComposite");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        auto ssaoPlugin = m_Renderer->GetPlugin<RenderPlugins::SSAO>(m_Info.SSAOPluginName);
        auto clusterPlugin = m_Renderer->GetPlugin<RenderPlugins::ClusteredLighting>(m_Info.ClusteredLightingPluginName);
        auto tlasPlugin = m_Renderer->GetPlugin<RenderPlugins::TLAS>(m_Info.TLASPluginName);
        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        auto objectDataBuffer = tlasPlugin->GetObjectBuffer();
        auto materialBuffer = materialsPlugin->GetMaterialBuffer();

        u32 arrayIndex = gBufferPlugin->GetArrayIndex();
        m_ResourceSet0->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet0->BindBuffer(1, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet0->BindBuffer(2, clusterPlugin->GetLightIndicesBuffer(), 0, clusterPlugin->GetLightIndicesBuffer()->GetAllocatedCount());
        m_ResourceSet0->BindBuffer(3, clusterPlugin->GetLightGridBuffer(), 0, clusterPlugin->GetLightGridBuffer()->GetAllocatedCount());
        m_ResourceSet0->BindBuffer(4, clusterPlugin->GetClusterDataBuffer(), 0, clusterPlugin->GetClusterDataBuffer()->GetAllocatedCount());
        m_ResourceSet0->BindTextureLayer(5, gBufferPlugin->GetNormalData(), arrayIndex, 0);
        m_ResourceSet0->BindTextureLayer(6, gBufferPlugin->GetColorData(), 0, 0);
        m_ResourceSet0->BindTextureLayer(7, gBufferPlugin->GetEmissiveData(), 0, 0);
        m_ResourceSet0->BindTextureLayer(8, gBufferPlugin->GetDepth(), arrayIndex, 0);
        if (data.EnvMap)
            m_ResourceSet0->BindTexture(9, data.EnvMap->GetBRDFTexture());
        else
            m_ResourceSet0->BindTextureLayer(9, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        m_ResourceSet0->BindTexture(10, m_Info.OutputTexture.get());
        m_ResourceSet0->BindTexture(11, m_Info.ReflectionsInputTexture.get());
        m_ResourceSet0->BindAccelerationStructure(12, tlasPlugin->GetAccelStructure());
        m_ResourceSet0->BindTexture(13, ssaoPlugin->GetOutputTexture());
        m_ResourceSet0->FlushBindings();

        m_ResourceSet1->BindBuffer(0, objectDataBuffer, 0, objectDataBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(1, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        m_ResourceSet1->FlushBindings();

        m_GroupTable->BindRayGenGroup(0);
        m_GroupTable->BindHitGroup(1, 0);
        m_GroupTable->BindMissGroup(2, 0);

        m_PushConstants.SSAOEnable = data.Settings.SSAOEnable;

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindRayTracingPipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet0.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(m_ResourceSet1.get(), 1);
        encoder->FlushResourceSet(1);
        encoder->BindResourceSet(materialsPlugin->GetTexturesSet(), 2);
        encoder->FlushResourceSet(2);
        encoder->PushConstants(0, sizeof(PushConstants), &m_PushConstants);
        encoder->TraceRays(
            m_GroupTable.get(),
            m_Info.OutputTexture->GetWidth(),
            m_Info.OutputTexture->GetHeight(),
            1
        );
        encoder->EndEncoding();
    }
}
