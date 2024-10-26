#include "hepch.h"
#include "RayReflections.h"

#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/RayTracing/RayTracingPipeline.h"
#include "Flourish/Api/RayTracing/RayTracingGroupTable.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/RayTracing/TLAS.h"
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/CollectMaterials.h"
#include "Heart/Renderer/Plugins/LightingData.h"
#include "Heart/Renderer/Plugins/GBuffer.h"

namespace Heart::RenderPlugins
{
    std::pair<int, int> Euclid(int a, int b)
    {
        if (!b)
            return { 1, 0 };
        int q = a / b;
        int r = a % b;
        auto st = Euclid(b, r);
        return { st.second, st.first - q * st.second };
    }

    void RayReflections::InitializeInternal()
    {
        // Queue shader loads 
        auto raygen = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_reflections/RayGen.rgen", true);
        auto miss = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_reflections/Miss.rmiss", true);
        auto hit = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_reflections/Hit.rchit", true);
        auto shadow = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/ray_reflections/Shadow.rmiss", true);
        Asset::LoadMany({ raygen, miss, hit, shadow }, false);

        Flourish::RayTracingPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.MaxRayRecursionDepth = 2;
        pipelineCreateInfo.Shaders = {
            { raygen->EnsureValid<ShaderAsset>()->GetShader() },
            { miss->EnsureValid<ShaderAsset>()->GetShader() },
            { hit->EnsureValid<ShaderAsset>()->GetShader() },
            { shadow->EnsureValid<ShaderAsset>()->GetShader() }
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

        // General Miss
        {
            Flourish::RayTracingShaderGroup group;
            group.GeneralShader = 1;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        // Closest hit
        {
            Flourish::RayTracingShaderGroup group;
            group.ClosestHitShader = 2;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        // Shadow miss
        {
            Flourish::RayTracingShaderGroup group;
            group.GeneralShader = 3;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        m_Pipeline = Flourish::RayTracingPipeline::Create(pipelineCreateInfo);

        Flourish::RayTracingGroupTableCreateInfo gtCreateInfo;
        gtCreateInfo.Pipeline = m_Pipeline;
        gtCreateInfo.MaxHitEntries = 1;
        gtCreateInfo.MaxMissEntries = 2;
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

    void RayReflections::ResizeInternal()
    {
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);

        // Half resolution
        m_GBufferMip = 1;

        m_PushData.HaltonData.p2 = ceil(log2(m_Info.OutputTexture->GetWidth()));
        m_PushData.HaltonData.p3 = ceil(log2(m_Info.OutputTexture->GetHeight())/log2(3));
        int w = pow(2, m_PushData.HaltonData.p2);
        int h = pow(3, m_PushData.HaltonData.p3);
        m_PushData.HaltonData.w = w;
        m_PushData.HaltonData.h = h;

        auto inv = Euclid(h, w);
        m_PushData.HaltonData.mX = h * ((inv.first < 0) ? (inv.first + w) : (inv.first % w));
        m_PushData.HaltonData.mY = w * ((inv.second < 0) ? (inv.second + h) : (inv.second % h));

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureWrite(m_Info.OutputTexture.get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer1().get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer2().get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBufferDepth().get());
            // .AccelStructure ???
    }

    void RayReflections::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RayReflections");

        auto tlasPlugin = m_Renderer->GetPlugin<RenderPlugins::TLAS>(m_Info.TLASPluginName);
        auto objectDataBuffer = tlasPlugin->GetObjectBuffer();
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        auto dirLightingBuffer = lightingDataPlugin->GetDirectionalBuffer();
        auto materialPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        auto materialBuffer = materialPlugin->GetMaterialBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);

        u32 arrayIndex = gBufferPlugin->GetArrayIndex();
        m_ResourceSet0->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet0->BindAccelerationStructure(1, tlasPlugin->GetAccelStructure());
        m_ResourceSet0->BindAccelerationStructure(2, lightingDataPlugin->GetLightTLAS());
        m_ResourceSet0->BindTexture(3, m_Info.OutputTexture.get());
        m_ResourceSet0->BindTextureLayer(4, gBufferPlugin->GetGBuffer2(), arrayIndex, m_GBufferMip);
        m_ResourceSet0->BindTextureLayer(5, gBufferPlugin->GetGBufferDepth(), arrayIndex, m_GBufferMip);
        m_ResourceSet0->FlushBindings();

        m_ResourceSet1->BindBuffer(0, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(1, dirLightingBuffer, 0, dirLightingBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(2, objectDataBuffer, 0, objectDataBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(3, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        if (data.EnvMap)
            m_ResourceSet1->BindTexture(4, data.EnvMap->GetEnvironmentCubemap());
        else
            m_ResourceSet1->BindTexture(4, m_Renderer->GetDefaultEnvironmentMap());
        m_ResourceSet1->FlushBindings();

        // Raygen
        m_GroupTable->BindRayGenGroup(0);

        // General intersections
        m_GroupTable->BindMissGroup(1, 0);
        m_GroupTable->BindHitGroup(2, 0);

        // Shadow intersections
        m_GroupTable->BindMissGroup(3, 1);

        // Set raycone spread angle based on camera FOV
        m_PushData.MipSpreadAngle = atanf(
            (2.f * tanf(data.Camera->GetFOV() * 0.5f)) / m_Renderer->GetRenderHeight()
        );
        
        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindRayTracingPipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet0.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(m_ResourceSet1.get(), 1);
        encoder->FlushResourceSet(1);
        encoder->BindResourceSet(materialPlugin->GetTexturesSet(), 2);
        encoder->FlushResourceSet(2);
        encoder->PushConstants(0, sizeof(PushData), &m_PushData);
        encoder->TraceRays(
            m_GroupTable.get(),
            m_Info.OutputTexture->GetWidth(),
            m_Info.OutputTexture->GetHeight(),
            1
        );
        encoder->EndEncoding();
    }
}
