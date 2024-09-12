#include "hepch.h"
#include "Bloom.h"

#include "Heart/Renderer/Plugins/TransparencyComposite.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"

namespace Heart::RenderPlugins
{
    void Bloom::InitializeInternal()
    {
        // Queue shader loads 
        auto downsampleShader = AssetManager::RetrieveAsset("engine/render_plugins/bloom/Downsample.comp", true);
        auto upsampleShader = AssetManager::RetrieveAsset("engine/render_plugins/bloom/Upsample.comp", true);
        auto compositeShader = AssetManager::RetrieveAsset("engine/render_plugins/bloom/Composite.comp", true);
        Asset::LoadMany({ downsampleShader, upsampleShader, compositeShader }, false);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { downsampleShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_DownsamplePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { upsampleShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_UpsamplePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.Shader = { compositeShader->EnsureValid<ShaderAsset>()->GetShader() };
        m_CompositePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_DownsampleResourceSet = m_DownsamplePipeline->CreateResourceSet(0, dsCreateInfo);
        m_UpsampleResourceSet = m_UpsamplePipeline->CreateResourceSet(0, dsCreateInfo);
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_CompositeResourceSet = m_CompositePipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void Bloom::ResizeInternal()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 7; // TODO: parameterize?
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Compute;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_DownsampleTexture = Flourish::Texture::Create(texCreateInfo);
        texCreateInfo.Width /= 2;
        texCreateInfo.Height /= 2;
        texCreateInfo.MipCount = 6; // TODO: parameterize?
        m_UpsampleTexture = Flourish::Texture::Create(texCreateInfo);
        
        m_MipCount = m_DownsampleTexture->GetMipCount();

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get());
        for (u32 i = 1; i < m_MipCount; i++)
        {
            m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddTextureRead(m_DownsampleTexture.get())
                .EncoderAddTextureWrite(m_DownsampleTexture.get());
            if (i == 1)
                m_GPUGraphNodeBuilder.EncoderAddTextureRead(m_Info.InputTexture.get());
        }
        for (int i = m_MipCount - 2; i >= 0; i--)
        {
            m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddTextureRead(m_DownsampleTexture.get())
                .EncoderAddTextureRead(m_UpsampleTexture.get())
                .EncoderAddTextureWrite(m_UpsampleTexture.get());
        }
        m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureRead(m_UpsampleTexture.get())
            .EncoderAddTextureRead(m_Info.InputTexture.get())
            .EncoderAddTextureWrite(m_Info.OutputTexture.get());
    }

    // TODO: resource sets could definitely be static
    void Bloom::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::Bloom");

        if (!data.Settings.BloomEnable)
        {
            for (u32 i = 1; i < m_MipCount; i++)
                m_CommandBuffer->EncodeComputeCommands()->EndEncoding();
            for (int i = m_MipCount - 2; i >= 0; i--)
                m_CommandBuffer->EncodeComputeCommands()->EndEncoding();
            m_CommandBuffer->EncodeComputeCommands()->EndEncoding();
            return;
        }

        // Downsample
        for (u32 i = 1; i < m_MipCount; i++)
        {
            m_PushData = {
                {
                    i == 1 ? m_Renderer->GetRenderWidth() : m_DownsampleTexture->GetMipWidth(i - 1),
                    i == 1 ? m_Renderer->GetRenderHeight() : m_DownsampleTexture->GetMipHeight(i - 1),
                },
                { m_DownsampleTexture->GetMipWidth(i), m_DownsampleTexture->GetMipHeight(i) },
                data.Settings.BloomThreshold,
                data.Settings.BloomKnee,
                data.Settings.BloomSampleScale,
                i == 1
            };

            // TODO: shouldn't force all bindings to be rewritten?
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->BindComputePipeline(m_DownsamplePipeline.get());
            if (i == 1)
                m_DownsampleResourceSet->BindTexture(0, m_Info.InputTexture.get());
            else
                m_DownsampleResourceSet->BindTextureLayer(0, m_DownsampleTexture.get(), 0, i - 1);
            m_DownsampleResourceSet->BindTextureLayer(1, m_DownsampleTexture.get(), 0, i);
            m_DownsampleResourceSet->FlushBindings();
            encoder->BindResourceSet(m_DownsampleResourceSet.get(), 0);
            encoder->FlushResourceSet(0);
            encoder->PushConstants(0, sizeof(SamplePushData), &m_PushData);
            encoder->Dispatch((m_PushData.DstResolution.x / 16) + 1, (m_PushData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }
        
        // Upsample
        for (int i = m_MipCount - 2; i >= 0; i--)
        {
            m_PushData = {
                { m_DownsampleTexture->GetMipWidth(i + 1), m_DownsampleTexture->GetMipHeight(i + 1) },
                { m_UpsampleTexture->GetMipWidth(i), m_UpsampleTexture->GetMipHeight(i) },
                data.Settings.BloomThreshold,
                data.Settings.BloomKnee,
                data.Settings.BloomSampleScale,
                false
            };

            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->BindComputePipeline(m_UpsamplePipeline.get());
            if (i == m_MipCount - 2)
                m_UpsampleResourceSet->BindTextureLayer(0, m_DownsampleTexture.get(), 0, i + 1);
            else
                m_UpsampleResourceSet->BindTextureLayer(0, m_UpsampleTexture.get(), 0, i + 1);
            m_UpsampleResourceSet->BindTextureLayer(1, m_DownsampleTexture.get(), 0, i);
            m_UpsampleResourceSet->BindTextureLayer(2, m_UpsampleTexture.get(), 0, i);
            m_UpsampleResourceSet->FlushBindings();

            encoder->BindResourceSet(m_UpsampleResourceSet.get(), 0);
            encoder->FlushResourceSet(0);
            encoder->PushConstants(0, sizeof(SamplePushData), &m_PushData);
            encoder->Dispatch((m_PushData.DstResolution.x / 16) + 1, (m_PushData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }

        m_CompositeResourceSet->BindTexture(0, m_UpsampleTexture.get());
        m_CompositeResourceSet->BindTexture(1, m_Info.InputTexture.get());
        m_CompositeResourceSet->BindTexture(2, m_Info.OutputTexture.get());
        m_CompositeResourceSet->FlushBindings();

        CompositePushData compPushData = {
            { m_Info.InputTexture->GetWidth(), m_Info.InputTexture->GetHeight() },
            { m_Info.OutputTexture->GetWidth(), m_Info.OutputTexture->GetHeight() },
            data.Settings.BloomStrength
        };

        // TODO: need specialization when input and output are different resolutions
        // b/c of imageLoad in shader
        
        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_CompositePipeline.get());
        encoder->BindResourceSet(m_CompositeResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(CompositePushData), &compPushData);
        encoder->Dispatch((compPushData.DstResolution.x / 16) + 1, (compPushData.DstResolution.y / 16) + 1, 1);
        encoder->EndEncoding();
    }
}
