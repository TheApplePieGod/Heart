#include "htpch.h"
#include "IndexBuffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanIndexBuffer.h"

namespace Heart
{
    Ref<IndexBuffer> IndexBuffer::Create(u32 indexCount)
    {
        return Create(indexCount, nullptr);
    }

    Ref<IndexBuffer> IndexBuffer::Create(u32 indexCount, u32* initialData)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create IndexBuffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanIndexBuffer>(indexCount, initialData); }
        }
    }
}