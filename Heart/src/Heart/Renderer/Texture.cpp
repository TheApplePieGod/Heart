#include "hepch.h"
#include "Texture.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanTexture.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"

namespace Heart
{
    Ref<Texture> Texture::Create(const TextureCreateInfo& createInfo, void* initialData)
    {
        HE_ENGINE_ASSERT(createInfo.Channels == 4, "Non 4 channel textures are not supported");

        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create texture: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanTexture>(createInfo, initialData); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLTexture>(createInfo, initialData); }
        }
    }

    void Texture::ScanForTransparency(int width, int height, int channels, void* data)
    {
        // TODO: change this possibly?
        if (m_Info.DataType != BufferDataType::UInt8) return;

        unsigned char* pixels = (unsigned char*)data;
        int size = width * height * channels;
        for (int i = 3; i < size; i += 4)
        {
            if (pixels[i] < 250)
            {
                if (pixels[i] < 5)
                    m_HasTransparency = true;
                else
                    m_HasTranslucency = true;
                return;
            }
        }
    }
}