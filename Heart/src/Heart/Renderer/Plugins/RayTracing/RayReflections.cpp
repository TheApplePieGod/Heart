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
    void RayReflections::Initialize()
    {
        Flourish::RayTracingPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.MaxRayRecursionDepth = 2;
        pipelineCreateInfo.Shaders = {
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/ray_tracing/ray_reflections/RayGen.rgen", true)->GetShader(),
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/ray_tracing/ray_reflections/Miss.rmiss", true)->GetShader(),
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/ray_tracing/ray_reflections/Hit.rchit", true)->GetShader(),
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/ray_tracing/ray_reflections/Shadow.rmiss", true)->GetShader()
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

        // TODO: need static
        Flourish::RayTracingGroupTableCreateInfo gtCreateInfo;
        gtCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
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

        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Compute;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        m_Output = Flourish::Texture::Create(texCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureWrite(m_Output.get())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer1())
            .EncoderAddTextureRead(gBufferPlugin->GetGBuffer2())
            .EncoderAddTextureRead(gBufferPlugin->GetGBufferDepth());
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
        auto materialPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        auto materialBuffer = materialPlugin->GetMaterialBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);
        
        if (!tlasPlugin->GetAccelStructure()->IsBuilt())
        {
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->EndEncoding();
            return;
        }

        u32 arrayIndex = gBufferPlugin->GetArrayIndex();
        m_ResourceSet0->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet0->BindAccelerationStructure(1, tlasPlugin->GetAccelStructure());
        m_ResourceSet0->BindTexture(2, m_Output.get());
        m_ResourceSet0->BindTextureLayer(3, gBufferPlugin->GetGBuffer1(), arrayIndex, 0);
        m_ResourceSet0->BindTextureLayer(4, gBufferPlugin->GetGBuffer2(), arrayIndex, 0);
        m_ResourceSet0->BindTextureLayer(5, gBufferPlugin->GetGBufferDepth(), arrayIndex, 0);
        m_ResourceSet0->FlushBindings();

        m_ResourceSet1->BindBuffer(0, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(1, objectDataBuffer, 0, objectDataBuffer->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(2, materialBuffer, 0, materialBuffer->GetAllocatedCount());
        if (data.EnvMap)
            m_ResourceSet1->BindTexture(3, data.EnvMap->GetEnvironmentCubemap());
        else
            m_ResourceSet1->BindTexture(3, m_Renderer->GetDefaultEnvironmentMap());
        m_ResourceSet1->FlushBindings();

        m_GroupTable->BindRayGenGroup(0);
        m_GroupTable->BindMissGroup(1, 0);
        m_GroupTable->BindHitGroup(2, 0);
        m_GroupTable->BindMissGroup(3, 1);

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindRayTracingPipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet0.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(m_ResourceSet1.get(), 1);
        encoder->FlushResourceSet(1);
        encoder->BindResourceSet(materialPlugin->GetTexturesSet(), 2);
        encoder->FlushResourceSet(2);
        encoder->TraceRays(
            m_GroupTable.get(),
            m_Renderer->GetRenderWidth(),
            m_Renderer->GetRenderHeight(),
            1
        );
        encoder->EndEncoding();
    }
}
