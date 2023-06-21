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
        m_ResourceSet = m_Pipeline->CreateResourceSet(0, dsCreateInfo);

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
        
        if (!tlasPlugin->GetAccelStructure()->IsBuilt())
        {
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->EndEncoding();
            return;
        }

        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindAccelerationStructure(1, tlasPlugin->GetAccelStructure());
        m_ResourceSet->BindTexture(2, m_Output.get());
        m_ResourceSet->FlushBindings();

        m_GroupTable->BindRayGenGroup(0);
        m_GroupTable->BindMissGroup(1, 0);
        m_GroupTable->BindHitGroup(2, 0);

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindRayTracingPipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->TraceRays(
            m_GroupTable.get(),
            m_Renderer->GetRenderWidth(),
            m_Renderer->GetRenderHeight(),
            1
        );
        encoder->EndEncoding();
    }
}
