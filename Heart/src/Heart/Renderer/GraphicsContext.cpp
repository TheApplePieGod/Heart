#include "htpch.h"
#include "GraphicsContext.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"

namespace Heart
{
    Ref<GraphicsContext> GraphicsContext::s_MainContext = nullptr;

    Ref<GraphicsContext> GraphicsContext::Create(void* window)
    {
        Ref<GraphicsContext> newContext = nullptr;
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create GraphicsContext: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { newContext = CreateRef<VulkanContext>(window); }
        }

        // first context created should be the main window, so that is set as the main context
        if (s_MainContext == nullptr)
            s_MainContext = newContext;

        return newContext;
    }
}