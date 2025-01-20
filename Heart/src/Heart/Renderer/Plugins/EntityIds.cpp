#include "hepch.h"
#include "EntityIds.h"

#include "glm/gtc/matrix_transform.hpp"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart::RenderPlugins
{
    void EntityIds::InitializeInternal()
    {
        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        ResizeInternal();
    }

    void EntityIds::ResizeInternal()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_Renderer->GetRenderWidth();
        texCreateInfo.Height = m_Renderer->GetRenderHeight();
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        texCreateInfo.Usage = Flourish::TextureUsageFlags::Graphics | Flourish::TextureUsageFlags::Transfer;
        texCreateInfo.SamplerState.UVWWrap = { Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder, Flourish::SamplerWrapMode::ClampToBorder };
        texCreateInfo.Format = Flourish::ColorFormat::R32_FLOAT;
        m_Texture = Flourish::Texture::Create(texCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPURead;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Generic;
        bufCreateInfo.Stride = sizeof(float);
        bufCreateInfo.ElementCount = m_Renderer->GetRenderWidth() * m_Renderer->GetRenderHeight();
        m_Buffer = Flourish::Buffer::Create(bufCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Transfer)
            .EncoderAddBufferWrite(m_Buffer.get())
            .EncoderAddTextureRead(m_Texture.get());
    }

    void EntityIds::RenderInternal(const SceneRenderData& data)
    {
        auto encoder = m_CommandBuffer->EncodeTransferCommands();
        if (data.Settings.CopyEntityIdsTextureToCPU)
        {
            encoder->CopyTextureToBuffer(m_Texture.get(), m_Buffer.get());

            // Lazy flushing because we don't care about immediate / accurate results
            m_Buffer->Flush();
        }
        encoder->EndEncoding();
    }
}
