#include "htpch.h"
#include "Texture.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"

namespace Heart
{
    Ref<Texture> Texture::Create(const std::string& path, bool floatComponents, int width, int height, int channels, void* data)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create texture: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanTexture>(path, floatComponents, width, height, channels, data); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLTexture>(path, floatComponents, width, height, channels, data); }
        }
    }

    void Texture::ScanForTransparency(int width, int height, int channels, void* data)
    {
        // TODO: change this possibly?
        if (m_FloatComponents) return;

        unsigned char* pixels = (unsigned char*)data;
        int size = width * height * channels;
        for (int i = 3; i < size; i += 4)
        {
            if (pixels[i] < 250)
            {
                m_HasTransparency = true;
                return;
            }
        }
    }
}