#include "hepch.h"
#include "GraphicsContext.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/OpenGL/OpenGLContext.h"

namespace Heart
{
    Ref<GraphicsContext> GraphicsContext::Create(void* window)
    {
        HE_ENGINE_LOG_TRACE("Creating GraphicsContext with api type {0}", RenderApi::TypeStrings[static_cast<u16>(Renderer::GetApiType())]);

        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create GraphicsContext: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanContext>(window); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLContext>(window); }
        }
    }
}