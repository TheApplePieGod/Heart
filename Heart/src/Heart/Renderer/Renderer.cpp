#include "htpch.h"
#include "Renderer.h"

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Platform/Vulkan/VulkanRenderApi.h"

namespace Heart
{
    Scope<RenderApi> Renderer::s_RenderApi;
    RenderApi::Type Renderer::s_RenderApiType;
    bool Renderer::s_UseReverseDepth = true;

    void Renderer::Initialize(RenderApi::Type apiType)
    {
        HE_ENGINE_LOG_INFO("Initializing Renderer with api type {0}", RenderApi::TypeStrings[static_cast<u16>(apiType)]);

        s_RenderApiType = apiType;
        switch (apiType)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot initialize Renderer: selected ApiType is not supported"); } break;
            case RenderApi::Type::Vulkan:
            { s_RenderApi = CreateScope<VulkanRenderApi>(); } break;
        }
    }

    void Renderer::Shutdown()
    {
        
    }

    void Renderer::OnWindowResize(GraphicsContext& context, u32 width, u32 height)
    {
        s_RenderApi->ResizeWindow(context, width, height);
    }
}