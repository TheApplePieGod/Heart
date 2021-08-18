#include "htpch.h"
#include "FrameBuffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanFrameBuffer.h"

namespace Heart
{
    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferCreateInfo& createInfo)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HT_ENGINE_ASSERT(false, "Cannot create FrameBuffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanFrameBuffer>(createInfo); }
        }
    }
}