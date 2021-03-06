#include "hepch.h"
#include "Buffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"

namespace Heart
{
    u32 BufferLayout::CalculateStrideAndOffsets()
    {
        u32 stride = 0;

        for (auto& element : m_Elements)
        {
            element.Offset = stride;
            stride += element.CalculatedSize;
        }

        if (stride % 4 != 0)
            HE_ENGINE_LOG_WARN("Buffer layout of length {0} has stride {1} that is not four byte aligned", m_Elements.size(), stride);

        return stride;
    }

    void Buffer::SetElements(void* data, u32 elementCount, u32 elementOffset)
    {
        HE_ENGINE_ASSERT(elementCount + elementOffset <= m_AllocatedCount, "Attempting to set data on buffer which is larger than allocated size");
        SetBytes(data, m_Layout.GetStride() * elementCount, m_Layout.GetStride() * elementOffset);
    }

    Ref<Buffer> Buffer::Create(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount)
    {
        return Create(type, usage, layout, elementCount, nullptr);
    }

    Ref<Buffer> Buffer::Create(Type type, BufferUsageType usage, const BufferLayout& layout, u32 elementCount, void* initialData)
    {
        HE_ENGINE_LOG_TRACE("Creating {0} buffer with {1} elements and {2}b stride", TypeStrings[static_cast<u16>(type)], elementCount, layout.GetStride());
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create Buffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanBuffer>(type, usage, layout, elementCount, initialData); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLBuffer>(type, usage, layout, elementCount, initialData); }
        }
    }

    Ref<Buffer> Buffer::CreateIndexBuffer(BufferUsageType usage, u32 elementCount)
    {
        return CreateIndexBuffer(usage, elementCount, nullptr);
    }

    Ref<Buffer> Buffer::CreateIndexBuffer(BufferUsageType usage, u32 elementCount, void* initialData)
    {
        BufferLayout layout = {
            { BufferDataType::UInt }
        };

        return Create(Type::Index, usage, layout, elementCount, initialData);
    }
}