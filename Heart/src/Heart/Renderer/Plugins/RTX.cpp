#include "hepch.h"
#include "RTX.h"

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
#include "Heart/Renderer/Plugins/TLAS.h"
#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/LightingData.h"

namespace Heart::RenderPlugins
{
    void RTX::Initialize()
    {
        Flourish::RayTracingPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.MaxRayRecursionDepth = 1;
        pipelineCreateInfo.Shaders = {
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/rtx/RayGen.rgen", true)->GetShader(),
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/rtx/Miss.rmiss", true)->GetShader(),
            AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/rtx/Hit.rchit", true)->GetShader()
        };
        
        // RayGen
        {
            Flourish::RayTracingShaderGroup group;
            group.GeneralShader = 0;
            pipelineCreateInfo.Groups.emplace_back(group);
        }

        // Miss
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

        m_Pipeline = Flourish::RayTracingPipeline::Create(pipelineCreateInfo);

        // TODO: need static
        Flourish::RayTracingGroupTableCreateInfo gtCreateInfo;
        gtCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        gtCreateInfo.Pipeline = m_Pipeline;
        gtCreateInfo.MaxHitEntries = 5;
        gtCreateInfo.MaxMissEntries = 5;
        gtCreateInfo.MaxCallableEntries = 5;
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

    void RTX::ResizeInternal()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        m_Output = Flourish::Texture::Create(texCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureWrite(m_Output.get());
            // .AccelStructure ???
    }

    void RTX::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::RTX");

        auto tlasPlugin = m_Renderer->GetPlugin<RenderPlugins::TLAS>(m_Info.TLASPluginName);
        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto lightingDataPlugin = m_Renderer->GetPlugin<RenderPlugins::LightingData>(m_Info.LightingDataPluginName);
        auto lightingDataBuffer = lightingDataPlugin->GetBuffer();
        
        if (!tlasPlugin->GetAccelStructure()->IsBuilt())
        {
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->EndEncoding();
            return;
        }

        m_ResourceSet0->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet0->BindAccelerationStructure(1, tlasPlugin->GetAccelStructure());
        m_ResourceSet0->BindTexture(2, m_Output.get());
        m_ResourceSet0->FlushBindings();

        m_ResourceSet1->BindBuffer(0, tlasPlugin->GetObjectBuffer(), 0, tlasPlugin->GetObjectBuffer()->GetAllocatedCount());
        m_ResourceSet1->BindBuffer(1, tlasPlugin->GetMaterialBuffer(), 0, tlasPlugin->GetMaterialBuffer()->GetAllocatedCount());
        if (data.EnvMap)
        {
            m_ResourceSet1->BindTexture(2, data.EnvMap->GetEnvironmentCubemap());
            m_ResourceSet1->BindTexture(3, data.EnvMap->GetIrradianceCubemap());
            m_ResourceSet1->BindTexture(4, data.EnvMap->GetPrefilterCubemap());
            m_ResourceSet1->BindTexture(5, data.EnvMap->GetBRDFTexture());
        }
        else
        {
            m_ResourceSet1->BindTexture(2, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet1->BindTexture(3, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet1->BindTexture(4, m_Renderer->GetDefaultEnvironmentMap());
            m_ResourceSet1->BindTextureLayer(5, m_Renderer->GetDefaultEnvironmentMap(), 0, 0);
        }
        m_ResourceSet1->BindBuffer(6, lightingDataBuffer, 0, lightingDataBuffer->GetAllocatedCount());
        m_ResourceSet1->FlushBindings();

        m_GroupTable->BindRayGenGroup(0);
        m_GroupTable->BindMissGroup(1, 0);
        m_GroupTable->BindHitGroup(2, 0);

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindRayTracingPipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet0.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->BindResourceSet(m_ResourceSet1.get(), 1);
        encoder->FlushResourceSet(1);
        encoder->BindResourceSet(tlasPlugin->GetTexturesSet(), 2);
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
