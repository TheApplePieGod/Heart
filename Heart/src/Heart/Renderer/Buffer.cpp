#include "htpch.h"
#include "Buffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"

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

    Ref<Buffer> Buffer::Create(Type type, const BufferLayout& layout, u32 elementCount)
    {
        return Create(type, layout, elementCount, nullptr);
    }

    Ref<Buffer> Buffer::Create(Type type, const BufferLayout& layout, u32 elementCount, void* initialData)
    {
        HE_ENGINE_LOG_TRACE("Creating {0} buffer with {1} elements and {2}b stride", TypeStrings[static_cast<u16>(type)], elementCount, layout.GetStride());
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create Buffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanBuffer>(type, layout, elementCount, initialData); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLBuffer>(type, layout, elementCount, initialData); }
        }
    }

    Ref<Buffer> Buffer::CreateIndexBuffer(u32 elementCount)
    {
        return CreateIndexBuffer(elementCount, nullptr);
    }

    Ref<Buffer> Buffer::CreateIndexBuffer(u32 elementCount, void* initialData)
    {
        BufferLayout layout = {
            { BufferDataType::UInt }
        };

        return Create(Type::Index, layout, elementCount, initialData);
    }
}