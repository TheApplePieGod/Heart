#include "htpch.h"
#include "Texture.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"

namespace Heart
{
    Ref<Texture> Texture::Create(const std::string& path, int width, int height, int channels, void* data)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create texture: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanTexture>(path, width, height, channels, data); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLTexture>(path, width, height, channels, data); }
        }
    }
}