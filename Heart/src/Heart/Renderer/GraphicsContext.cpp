#include "htpch.h"
#include "GraphicsContext.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"

namespace Heart
{
    Ref<GraphicsContext> GraphicsContext::Create(void* window)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create GraphicsContext: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanContext>(window); }
        }
    }
}