#include "htpch.h"
#include "GraphicsContext.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"

namespace Heart
{
    Scope<GraphicsContext> GraphicsContext::Create(RenderApi::Type apiType, void* window)
    {
        switch (apiType)
        {
            default:
            { HT_ENGINE_ASSERT(false, "Selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateScope<VulkanContext>(window); }
        }
    }
}