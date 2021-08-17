#include "htpch.h"
#include "Renderer.h"

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Platform/Vulkan/VulkanRenderApi.h"

namespace Heart
{
    Scope<RenderApi> Renderer::s_RenderApi;
    RenderApi::Type Renderer::s_RenderApiType;

    void Renderer::Initialize(RenderApi::Type apiType)
    {
        s_RenderApiType = apiType;
        switch (apiType)
        {
            default:
            { HT_ENGINE_ASSERT(false, "Cannot initialize Renderer: selected ApiType is not supported"); } break;
            case RenderApi::Type::Vulkan:
            { s_RenderApi = CreateScope<VulkanRenderApi>(); } break;
        }
    }

    void Renderer::Shutdown()
    {
        
    }
}