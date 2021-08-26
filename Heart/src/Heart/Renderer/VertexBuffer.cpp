#include "htpch.h"
#include "VertexBuffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanVertexBuffer.h"

namespace Heart
{
    Ref<VertexBuffer> VertexBuffer::Create(const BufferLayout& layout, u32 elementCount)
    {
        return Create(layout, elementCount, nullptr);
    }

    Ref<VertexBuffer> VertexBuffer::Create(const BufferLayout& layout, u32 elementCount, void* initialData)
    {
        HE_ENGINE_LOG_TRACE("Creating vertex buffer with {0} elements and {1}b stride", elementCount, layout.GetStride());
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create VertexBuffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanVertexBuffer>(layout, elementCount, initialData); }
        }
    }
}