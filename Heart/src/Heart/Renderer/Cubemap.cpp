#include "htpch.h"
#include "Cubemap.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanCubemap.h"

namespace Heart
{
    Ref<Cubemap> Cubemap::Create(int width, int height, bool floatComponents)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create cubemap: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanCubemap>(width, height, floatComponents); }
            //case RenderApi::Type::OpenGL:
            //{ return CreateRef<OpenGLTexture>(path, floatComponents, width, height, channels, data); }
        }
    }
}