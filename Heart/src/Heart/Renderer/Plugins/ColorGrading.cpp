#include "hepch.h"
#include "ColorGrading.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void ColorGrading::InitializeInternal()
    {
        // Queue shader loads 
        auto compShader = AssetManager::RetrieveAsset("engine/render_plugins/color_grading/Composite.comp", true);
        Asset::LoadMany({ compShader }, false);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { compShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_Pipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = m_Pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void ColorGrading::ResizeInternal()
    {
        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureRead(m_Info.InputTexture.get())
            .EncoderAddTextureWrite(m_Info.OutputTexture.get());
    }

    void ColorGrading::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ColorGrading");

        // TODO: need specialization when input and output are different resolutions
        // b/c of imageLoad in shader

        // TODO: this could probably be static
        m_ResourceSet->BindTexture(0, m_Info.InputTexture.get());
        m_ResourceSet->BindTexture(1, m_Info.OutputTexture.get());
        m_ResourceSet->FlushBindings();
        
        PushConstants consts = {
            { m_Info.InputTexture->GetWidth(), m_Info.InputTexture->GetHeight() },
            { m_Info.OutputTexture->GetWidth(), m_Info.OutputTexture->GetHeight() },
            data.Settings.TonemapEnable
        };

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(PushConstants), &consts);
        encoder->Dispatch((consts.DstResolution.x / 16) + 1, (consts.DstResolution.y / 16) + 1, 1);
        encoder->EndEncoding();
    }
}
