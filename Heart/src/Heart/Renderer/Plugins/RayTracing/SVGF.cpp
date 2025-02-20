#include "hepch.h"
#include "SVGF.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/GBuffer.h"
#include "Heart/Renderer/Plugins/FrameData.h"
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
    u32 SVGF::GetArrayIndex() const
    {
        return Flourish::Context::FrameCount() % 2;
    }

    u32 SVGF::GetPrevArrayIndex() const
    {
        return (Flourish::Context::FrameCount() - 1) % 2;
    }

    void SVGF::InitializeInternal()
    {
        // Queue shader loads 
        auto reprojShader = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/svgf/Reprojection.comp", true);
        auto atrousShader = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/svgf/ATrous.comp", true);
        auto upsampleShader = AssetManager::RetrieveAsset("engine/render_plugins/ray_tracing/svgf/Upsample.comp", true);
        Asset::LoadMany({ reprojShader, atrousShader, upsampleShader }, false);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { reprojShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_TemporalPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { atrousShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_ATrousPipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { upsampleShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_UpsamplePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        m_TemporalResourceSet = m_TemporalPipeline->CreateResourceSet(0, dsCreateInfo);
        m_UpsampleResourceSet = m_UpsamplePipeline->CreateResourceSet(0, dsCreateInfo);
        m_ATrousResourceSet = m_ATrousPipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void SVGF::ResizeInternal()
    {
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);

        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Info.InputTexture->GetWidth() * m_Info.InputUsableFactorWidth;
        texCreateInfo.Height = m_Info.InputTexture->GetHeight() * m_Info.InputUsableFactorHeight;
        texCreateInfo.ArrayCount = 2;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Compute;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        texCreateInfo.SamplerState.AnisotropyEnable = true;
        m_ColorHistory = Flourish::Texture::Create(texCreateInfo);
        m_MomentsHistory = Flourish::Texture::Create(texCreateInfo);
        m_TempTexture = Flourish::Texture::Create(texCreateInfo);
        m_DebugTexture = Flourish::Texture::Create(texCreateInfo);

        // Force temporal refresh
        m_TemporalPushData.ShouldReset = true;

        // Half resolution
        m_GBufferMip = 1;

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureRead(m_Info.InputTexture.get())
            .EncoderAddTextureRead(gBufferPlugin->GetNormalData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetColorData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetDepth().get())
            .EncoderAddTextureWrite(m_MomentsHistory.get())
            .EncoderAddTextureWrite(m_ColorHistory.get());

        for (u32 i = 0; i < m_ATrousIterations; i++)
        {
            m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddTextureRead(gBufferPlugin->GetNormalData().get())
                .EncoderAddTextureRead(gBufferPlugin->GetColorData().get())
                .EncoderAddTextureRead(gBufferPlugin->GetDepth().get())
                .EncoderAddTextureWrite(m_TempTexture.get());

            Flourish::Texture* read = m_TempTexture.get();
            if (i == 0)
                read = m_ColorHistory.get();
            m_GPUGraphNodeBuilder.EncoderAddTextureRead(read);
        }

        m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureRead(gBufferPlugin->GetNormalData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetColorData().get())
            .EncoderAddTextureRead(gBufferPlugin->GetDepth().get())
            .EncoderAddTextureRead(m_TempTexture.get())
            .EncoderAddTextureWrite(m_Info.OutputTexture.get());
    }

    // TODO: resource sets could definitely be static
    void SVGF::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::SVGF");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto gBufferPlugin = m_Renderer->GetPlugin<RenderPlugins::GBuffer>(m_Info.GBufferPluginName);

        u32 arrayIndex = gBufferPlugin->GetArrayIndex();
        u32 prevArrayIndex = gBufferPlugin->GetPrevArrayIndex();
        m_TemporalResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_TemporalResourceSet->BindTextureLayer(1, gBufferPlugin->GetNormalData(), prevArrayIndex, m_GBufferMip);
        m_TemporalResourceSet->BindTextureLayer(3, gBufferPlugin->GetDepth(), prevArrayIndex, m_GBufferMip);
        m_TemporalResourceSet->BindTextureLayer(4, gBufferPlugin->GetNormalData(), arrayIndex, m_GBufferMip);
        m_TemporalResourceSet->BindTextureLayer(5, gBufferPlugin->GetColorData(), 0, m_GBufferMip);
        m_TemporalResourceSet->BindTextureLayer(6, gBufferPlugin->GetDepth(), arrayIndex, m_GBufferMip);
        m_TemporalResourceSet->BindTextureLayer(7, m_ColorHistory.get(), GetPrevArrayIndex(), 0);
        m_TemporalResourceSet->BindTextureLayer(8, m_MomentsHistory.get(), GetPrevArrayIndex(), 0);
        m_TemporalResourceSet->BindTexture(9, m_Info.InputTexture.get());
        m_TemporalResourceSet->BindTextureLayer(10, m_ColorHistory.get(), GetArrayIndex(), 0);
        m_TemporalResourceSet->BindTextureLayer(11, m_MomentsHistory.get(), GetArrayIndex(), 0);
        m_TemporalResourceSet->BindTextureLayer(12, m_DebugTexture.get(), GetArrayIndex(), 0);
        m_TemporalResourceSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_TemporalPipeline.get());
        encoder->BindResourceSet(m_TemporalResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(TemporalPushData), &m_TemporalPushData);
        encoder->Dispatch((m_ColorHistory->GetWidth() / 16) + 1, (m_ColorHistory->GetHeight() / 16) + 1, 1);
        encoder->EndEncoding();

        for (u32 i = 0; i < m_ATrousIterations; i++)
        {
            m_ATrousPushData.Iteration = i;
            m_ATrousResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
            m_ATrousResourceSet->BindTextureLayer(1, gBufferPlugin->GetNormalData(), arrayIndex, m_GBufferMip);
            m_ATrousResourceSet->BindTextureLayer(2, gBufferPlugin->GetColorData(), 0, m_GBufferMip);
            m_ATrousResourceSet->BindTextureLayer(3, gBufferPlugin->GetDepth(), arrayIndex, m_GBufferMip);
            if (i == 0)
                m_ATrousResourceSet->BindTextureLayer(4, m_ColorHistory.get(), GetArrayIndex(), 0);
            else
                m_ATrousResourceSet->BindTextureLayer(4, m_TempTexture.get(), i % 2, 0);
            m_ATrousResourceSet->BindTextureLayer(5, m_TempTexture.get(), (i + 1) % 2, 0);
            m_ATrousResourceSet->FlushBindings();
            encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->BindComputePipeline(m_ATrousPipeline.get());
            encoder->BindResourceSet(m_ATrousResourceSet.get(), 0);
            encoder->FlushResourceSet(0);
            encoder->PushConstants(0, sizeof(ATrousPushData), &m_ATrousPushData);
            encoder->Dispatch((m_TempTexture->GetWidth() / 16) + 1, (m_TempTexture->GetHeight() / 16) + 1, 1);
            encoder->EndEncoding();
        }

        m_UpsampleResourceSet->BindTextureLayer(0, gBufferPlugin->GetNormalData(), arrayIndex, m_GBufferMip);
        m_UpsampleResourceSet->BindTextureLayer(1, gBufferPlugin->GetDepth(), arrayIndex, m_GBufferMip);
        m_UpsampleResourceSet->BindTextureLayer(2, gBufferPlugin->GetNormalData(), arrayIndex, 0);
        m_UpsampleResourceSet->BindTextureLayer(3, gBufferPlugin->GetColorData(), 0, 0);
        m_UpsampleResourceSet->BindTextureLayer(4, gBufferPlugin->GetDepth(), arrayIndex, 0);
        m_UpsampleResourceSet->BindTextureLayer(5, m_TempTexture.get(), m_ATrousIterations % 2, 0);
        m_UpsampleResourceSet->BindTexture(6, m_Info.OutputTexture.get());
        m_UpsampleResourceSet->FlushBindings();
        encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_UpsamplePipeline.get());
        encoder->BindResourceSet(m_UpsampleResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->Dispatch((m_Info.OutputTexture->GetWidth() / 16) + 1, (m_Info.OutputTexture->GetHeight() / 16) + 1, 1);
        encoder->EndEncoding();

        m_TemporalPushData.ShouldReset = false;
    }
}
