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
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"

namespace Heart::RenderPlugins
{
    void Bloom::Initialize()
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.ColorAttachments.push_back({
            m_Renderer->GetRenderTexture()->GetColorFormat(),
            Flourish::AttachmentInitialization::Preserve
        });
        rpCreateInfo.Subpasses.push_back({
            {},
            { { Flourish::SubpassAttachmentType::Color, 0 } }
        });
        m_RenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/bloom/Composite.frag", true)->GetShader();
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/FullscreenTriangle.vert", true)->GetShader();
        pipelineCreateInfo.VertexInput = false;
        pipelineCreateInfo.BlendStates = {
            { true, Flourish::BlendFactor::SrcAlpha, Flourish::BlendFactor::DstAlpha, Flourish::BlendFactor::One, Flourish::BlendFactor::Zero }
        };
        pipelineCreateInfo.DepthConfig.DepthTest = false;
        pipelineCreateInfo.DepthConfig.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::Backface;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::CounterClockwise;
        auto pipeline = m_RenderPass->CreatePipeline("main", pipelineCreateInfo);

        Flourish::ComputePipelineCreateInfo compCreateInfo;
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/bloom/Downsample.comp", true)->GetShader();
        m_DownsamplePipeline = Flourish::ComputePipeline::Create(compCreateInfo);
        compCreateInfo.ComputeShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/render_plugins/bloom/Upsample.comp", true)->GetShader();
        m_UpsamplePipeline = Flourish::ComputePipeline::Create(compCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::MultiPerFrame;
        m_DownsampleResourceSet = m_DownsamplePipeline->CreateResourceSet(0, dsCreateInfo);
        m_UpsampleResourceSet = m_UpsamplePipeline->CreateResourceSet(0, dsCreateInfo);
        dsCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_CompositeResourceSet = pipeline->CreateResourceSet(0, dsCreateInfo);

        ResizeInternal();
    }

    void Bloom::ResizeInternal()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_RenderPass;
        fbCreateInfo.Width = m_Renderer->GetRenderWidth();
        fbCreateInfo.Height = m_Renderer->GetRenderHeight();
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_Renderer->GetRenderTexture() });
        m_Framebuffer = Flourish::Framebuffer::Create(fbCreateInfo);

        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 7; // TODO: parameterize?
        texCreateInfo.Format = Flourish::ColorFormat::RGBA16_FLOAT;
        texCreateInfo.Usage = Flourish::TextureUsageType::ComputeTarget;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge, Flourish::SamplerWrapMode::ClampToEdge };
        m_DownsampleTexture = Flourish::Texture::Create(texCreateInfo);
        m_UpsampleTexture = Flourish::Texture::Create(texCreateInfo);
        
        m_MipCount = m_DownsampleTexture->GetMipCount();

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(BloomData);
        bufCreateInfo.ElementCount = m_MipCount * 2;
        m_DataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get());
        for (u32 i = 1; i < m_MipCount; i++)
        {
            m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddTextureRead(m_DownsampleTexture.get())
                .EncoderAddTextureWrite(m_DownsampleTexture.get());
            if (i == 1)
                m_GPUGraphNodeBuilder.EncoderAddTextureRead(m_Renderer->GetRenderTexture().get());
        }
        for (u32 i = m_MipCount - 2; i > 0; i--)
        {
            m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddTextureRead(m_DownsampleTexture.get())
                .EncoderAddTextureRead(m_UpsampleTexture.get())
                .EncoderAddTextureWrite(m_UpsampleTexture.get());
        }
        m_GPUGraphNodeBuilder.AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddFramebuffer(m_Framebuffer.get())
            .EncoderAddTextureRead(m_UpsampleTexture.get());

    }

    // TODO: resource sets could definitely be static
    void Bloom::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::Bloom");

        // Downsample
        for (u32 i = 1; i < m_MipCount; i++)
        {
            BloomData bloomData = {
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
            m_DataBuffer->SetElements(&bloomData, 1, i - 1);

            // TODO: shouldn't force all bindings to be rewritten?
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->BindPipeline(m_DownsamplePipeline.get());
            if (i == 1)
            {
                m_DownsampleResourceSet->BindBuffer(0, m_DataBuffer.get(), 0, 1);
                m_DownsampleResourceSet->BindTexture(1, m_Renderer->GetRenderTexture().get());
                m_DownsampleResourceSet->BindTextureLayer(2, m_DownsampleTexture.get(), 0, i);
                m_DownsampleResourceSet->FlushBindings();
            }
            else
            {
                m_DownsampleResourceSet->BindBuffer(0, m_DataBuffer.get(), 0, 1);
                m_DownsampleResourceSet->BindTextureLayer(1, m_DownsampleTexture.get(), 0, i - 1);
                m_DownsampleResourceSet->BindTextureLayer(2, m_DownsampleTexture.get(), 0, i);
                m_DownsampleResourceSet->FlushBindings();
            }
            encoder->BindResourceSet(m_DownsampleResourceSet.get(), 0);
            encoder->UpdateDynamicOffset(0, 0, (i - 1) * sizeof(BloomData));
            encoder->FlushResourceSet(0);
            encoder->Dispatch((bloomData.DstResolution.x / 16) + 1, (bloomData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }
        
        // Upsample
        for (u32 i = m_MipCount - 2; i > 0; i--)
        {
            BloomData bloomData = {
                { m_DownsampleTexture->GetMipWidth(i + 1), m_DownsampleTexture->GetMipHeight(i + 1) },
                { m_UpsampleTexture->GetMipWidth(i), m_UpsampleTexture->GetMipHeight(i) },
                data.Settings.BloomThreshold,
                data.Settings.BloomKnee,
                data.Settings.BloomSampleScale,
                false
            };
            m_DataBuffer->SetElements(&bloomData, 1, i + m_MipCount);

            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            encoder->BindPipeline(m_UpsamplePipeline.get());
            if (i == m_MipCount - 2)
            {
                m_UpsampleResourceSet->BindBuffer(0, m_DataBuffer.get(), 0, 1);
                m_UpsampleResourceSet->BindTextureLayer(1, m_DownsampleTexture.get(), 0, i + 1);
                m_UpsampleResourceSet->BindTextureLayer(2, m_DownsampleTexture.get(), 0, i);
                m_UpsampleResourceSet->BindTextureLayer(3, m_UpsampleTexture.get(), 0, i);
                m_UpsampleResourceSet->FlushBindings();
            }
            else
            {
                m_UpsampleResourceSet->BindBuffer(0, m_DataBuffer.get(), 0, 1);
                m_UpsampleResourceSet->BindTextureLayer(1, m_UpsampleTexture.get(), 0, i + 1);
                m_UpsampleResourceSet->BindTextureLayer(2, m_DownsampleTexture.get(), 0, i);
                m_UpsampleResourceSet->BindTextureLayer(3, m_UpsampleTexture.get(), 0, i);
                m_UpsampleResourceSet->FlushBindings();
            }

            encoder->BindResourceSet(m_UpsampleResourceSet.get(), 0);
            encoder->UpdateDynamicOffset(0, 0, (i + m_MipCount) * sizeof(BloomData));
            encoder->FlushResourceSet(0);

            encoder->Dispatch((bloomData.DstResolution.x / 16) + 1, (bloomData.DstResolution.y / 16) + 1, 1);
            encoder->EndEncoding();
        }

        m_CompositeResourceSet->BindTexture(0, m_UpsampleTexture.get());
        m_CompositeResourceSet->FlushBindings();
        
        auto encoder = m_CommandBuffer->EncodeRenderCommands(m_Framebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindResourceSet(m_CompositeResourceSet.get(), 0);
        encoder->FlushResourceSet(0);

        encoder->Draw(3, 0, 1, 0);

        encoder->EndEncoding();
    }
}
