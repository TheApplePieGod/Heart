#include "htpch.h"
#include "Buffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"

namespace Heart
{
    u32 BufferLayout::CalculateStride()
    {
        u32 stride = 0;

        for (auto& element : m_Elements)
            stride += element.CalculatedSize;

        if (stride % 4 != 0)
            HE_ENGINE_LOG_WARN("Buffer layout of length {0} has stride {1} that is not four byte aligned", m_Elements.size(), stride);

        return stride;
    }

    Ref<Buffer> Buffer::Create(const BufferLayout& layout, u32 elementCount)
    {
        return Create(layout, elementCount, nullptr);
    }

    Ref<Buffer> Buffer::Create(const BufferLayout& layout, u32 elementCount, void* initialData)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create Buffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanBuffer>(layout, elementCount, initialData, VulkanBuffer::Type::Uniform); }
        }
    }

    Ref<Buffer> BigBuffer::Create(const BufferLayout& layout, u32 elementCount)
    {
        return Create(layout, elementCount, nullptr);
    }

    Ref<Buffer> BigBuffer::Create(const BufferLayout& layout, u32 elementCount, void* initialData)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create Buffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanBuffer>(layout, elementCount, initialData, VulkanBuffer::Type::Storage); }
        }
    }
}