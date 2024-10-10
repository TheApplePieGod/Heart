#include "hepch.h"
#include "SSAO.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ComputePipeline.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/Buffer.h"

const auto OUTPUT_FORMAT = Flourish::ColorFormat::R16_FLOAT;

namespace Heart::RenderPlugins
{
    void SSAO::InitializeInternal()
    {
        auto shader = AssetManager::RetrieveAsset("engine/render_plugins/ssao/Compute.comp", true);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.Shader = { shader->EnsureValid<ShaderAsset>()->GetShader() };
        m_Pipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = m_Pipeline->CreateResourceSet(0, dsCreateInfo);
        
        // https://learnopengl.com/Advanced-Lighting/SSAO
        // Generate random hemispherical perturbation vectors
        std::uniform_real_distribution<float> floats(0.f, 1.f);
        std::default_random_engine generator;
        for (u32 i = 0; i < 64; i++)
        {
            m_PushConstants.Samples[i].x = floats(generator) * 2.f - 1.f;
            m_PushConstants.Samples[i].y = floats(generator) * 2.f - 1.f;
            m_PushConstants.Samples[i].z = floats(generator);
            m_PushConstants.Samples[i].w = 0.f;

            m_PushConstants.Samples[i] = glm::normalize(m_PushConstants.Samples[i]);
            m_PushConstants.Samples[i] *= floats(generator);

            // Scale sample to distribute more towards the origin
            float scale = (float)i / 64.f; 
            scale = 0.1f + (scale * scale * 0.9f); // lerp(0.1, 1.0, scale * scale)
            m_PushConstants.Samples[i] *= scale;
        }

        // Generate random noise for the sample kernels
        std::vector<glm::vec4> noise;
        for (u32 i = 0; i < 16; i++)
        {
            noise.emplace_back(
                floats(generator) * 2.f - 1.f, 
                floats(generator) * 2.f - 1.f, 
                0.f,
                0.f
            );
        }

        // Create a texture from the random noise for later sampling
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = 4;
        texCreateInfo.Height = 4;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA32_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Readonly;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.InitialData = noise.data();
        texCreateInfo.InitialDataSize = noise.size() * sizeof(glm::vec4);
        texCreateInfo.SamplerState.MinFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.MagFilter = Flourish::SamplerFilter::Nearest;
        texCreateInfo.SamplerState.UVWWrap = {
            Flourish::SamplerWrapMode::Repeat, Flourish::SamplerWrapMode::Repeat, Flourish::SamplerWrapMode::Repeat
        };
        m_NoiseTexture = Flourish::Texture::Create(texCreateInfo);

        ResizeInternal();
    }

    void SSAO::ResizeInternal()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Format = OUTPUT_FORMAT;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Compute;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_OutputTexture = Flourish::Texture::Create(texCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
            .EncoderAddTextureWrite(m_OutputTexture.get())
            .EncoderAddTextureRead(m_Info.InputDepthTexture.get())
            .EncoderAddTextureRead(m_Info.InputNormalsTexture.get());
    }

    void SSAO::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::SSAO");

        if (!data.Settings.SSAOEnable)
        {
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->EndEncoding();
            return;
        }

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindTexture(1, m_OutputTexture.get());
        m_ResourceSet->BindTexture(3, m_NoiseTexture.get());

        // TODO: fix for RT
        m_ResourceSet->BindTexture(2, m_Info.InputDepthTexture.get());
        m_ResourceSet->BindTexture(4, m_Info.InputNormalsTexture.get(), 0);

        m_ResourceSet->FlushBindings();

        m_PushConstants.KernelSize = data.Settings.SSAOKernelSize;
        m_PushConstants.Radius = data.Settings.SSAORadius;
        m_PushConstants.Bias = data.Settings.SSAOBias;
        m_PushConstants.RenderSize = { m_OutputTexture->GetWidth(), m_OutputTexture->GetHeight() };

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        encoder->BindComputePipeline(m_Pipeline.get());
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);
        encoder->PushConstants(0, sizeof(PushConstants), &m_PushConstants);
        encoder->Dispatch((m_OutputTexture->GetWidth() / 16) + 1, (m_OutputTexture->GetHeight() / 16) + 1, 1);
        encoder->EndEncoding();
    }
}
