#include "hepch.h"
#include "BlitTexture.h"

#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Core/Timing.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/GraphicsCommandEncoder.h"
#include "Flourish/Api/CommandBuffer.h"

namespace Heart::RenderPlugins
{
    void BlitTexture::InitializeInternal()
    {
        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        ResizeInternal();
    }

    void BlitTexture::ResizeInternal()
    {
        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Graphics)
            .EncoderAddTextureRead(m_Info.SrcTexture.get())
            .EncoderAddTextureWrite(m_Info.DstTexture.get());
    }

    void BlitTexture::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::BlitTexture");

        u32 srcLayerIndex = m_Info.SrcDynamicLayerIndex
            ? Flourish::Context::FrameCount() % m_Info.SrcLayerIndex
            : m_Info.SrcLayerIndex;

        u32 dstLayerIndex = m_Info.DstDynamicLayerIndex
            ? Flourish::Context::FrameCount() % m_Info.DstLayerIndex
            : m_Info.DstLayerIndex;

        auto encoder = m_CommandBuffer->EncodeGraphicsCommands();
        encoder->BlitTexture(
            m_Info.SrcTexture.get(),
            m_Info.DstTexture.get(),
            srcLayerIndex, m_Info.SrcMipLevel,
            dstLayerIndex, m_Info.DstMipLevel
        );
        encoder->EndEncoding();
    }
}
