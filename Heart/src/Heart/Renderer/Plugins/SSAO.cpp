#include "hepch.h"
#include "SSAO.h"

#include "Heart/Renderer/Plugins/FrameData.h"
#include "Heart/Renderer/Plugins/RenderMeshBatches.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Flourish/Api/RenderCommandEncoder.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/ResourceSet.h"
#include "Flourish/Api/Buffer.h"

const auto OUTPUT_FORMAT = Flourish::ColorFormat::R16_FLOAT;

namespace Heart::RenderPlugins
{
    void SSAO::Initialize()
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.ColorAttachments.push_back({
            OUTPUT_FORMAT,
            Flourish::AttachmentInitialization::Clear
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            { { Flourish::SubpassAttachmentType::Color, 0 } }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/ssao/Fragment.frag", true)->GetShader();
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/FullscreenTriangle.vert", true)->GetShader();
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.BlendStates = {{ false }};
        pipelineCreateInfo.DepthConfig.DepthTest = false;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::CounterClockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_ResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(SSAOData);
        bufCreateInfo.ElementCount = 1;
        m_DataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        
        // https://learnopengl.com/Advanced-Lighting/SSAO
        // Generate random hemispherical perturbation vectors
        std::uniform_real_distribution<float> floats(0.f, 1.f);
        std::default_random_engine generator;
        for (u32 i = 0; i < 64; i++)
        {
            m_Data.Samples[i].x = floats(generator) * 2.f - 1.f;
            m_Data.Samples[i].y = floats(generator) * 2.f - 1.f;
            m_Data.Samples[i].z = floats(generator);
            m_Data.Samples[i].w = 0.f;

            m_Data.Samples[i] = glm::normalize(m_Data.Samples[i]);
            m_Data.Samples[i] *= floats(generator);

            // Scale sample to distribute more towards the origin
            float scale = (float)i / 64.f; 
            scale = 0.1f + (scale * scale * 0.9f); // lerp(0.1, 1.0, scale * scale)
            m_Data.Samples[i] *= scale;
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
        texCreateInfo.Writability = Flourish::TextureWritability::Once;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA32_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::Readonly;
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
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Format = OUTPUT_FORMAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_OutputTexture = Flourish::Texture::Create(texCreateInfo);

        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_OutputTexture->GetWidth();
        fbCreateInfo.Height = m_OutputTexture->GetHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 1.f }, m_OutputTexture });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        auto meshPlugin = m_Renderer->GetPlugin<RenderPlugins::RenderMeshBatches>(m_Info.RenderMeshBatchesPluginName);
        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get())
            .EncoderAddTextureRead(m_Renderer->GetDepthTexture().get())
            .EncoderAddTextureRead(meshPlugin->GetNormalsTexture());
    }

    void SSAO::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::SSAO");

        auto frameDataPlugin = m_Renderer->GetPlugin<RenderPlugins::FrameData>(m_Info.FrameDataPluginName);
        auto frameDataBuffer = frameDataPlugin->GetBuffer();
        auto meshPlugin = m_Renderer->GetPlugin<RenderPlugins::RenderMeshBatches>(m_Info.RenderMeshBatchesPluginName);

        // TODO: this could probably be static
        m_ResourceSet->BindBuffer(0, frameDataBuffer, 0, 1);
        m_ResourceSet->BindBuffer(1, m_DataBuffer.get(), 0, 1);
        m_ResourceSet->BindTexture(2, m_Renderer->GetDepthTexture().get());
        m_ResourceSet->BindTexture(3, m_NoiseTexture.get());
        m_ResourceSet->BindTexture(4, meshPlugin->GetNormalsTexture());
        m_ResourceSet->FlushBindings();

        m_Data.KernelSize = data.Settings.SSAOKernelSize;
        m_Data.Radius = data.Settings.SSAORadius;
        m_Data.Bias = data.Settings.SSAOBias;
        m_Data.RenderSize = { m_OutputTexture->GetWidth(), m_OutputTexture->GetHeight() };
        m_DataBuffer->SetElements(&m_Data, 1, 0);

        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_ResourceSet.get(), 0);
        encoder->FlushResourceSet(0);

        encoder->Draw(3, 0, 1, 0);

        encoder->EndEncoding();
    }
}
