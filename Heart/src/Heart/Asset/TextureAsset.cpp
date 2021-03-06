#include "hepch.h"
#include "TextureAsset.h"

#include "Heart/Renderer/Texture.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    void TextureAsset::Load()
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        bool floatComponents = false;
        if (m_Extension == ".hdr") // environment map: use float components and flip on load
        {
            floatComponents = true;
            stbi_set_flip_vertically_on_load(true);
        }
        else
            stbi_set_flip_vertically_on_load(false);

        void* pixels = nullptr;
        int width, height, channels;
        if (floatComponents)
            pixels = stbi_loadf(m_AbsolutePath.c_str(), &width, &height, &channels, m_DesiredChannelCount);
        else
            pixels = stbi_load(m_AbsolutePath.c_str(), &width, &height, &channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_AbsolutePath);
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        TextureCreateInfo createInfo = {
            static_cast<u32>(width), static_cast<u32>(height), static_cast<u32>(m_DesiredChannelCount),
            floatComponents ? BufferDataType::Float : BufferDataType::UInt8,
            BufferUsageType::Static,
            1, 0
        };
        m_Texture = Texture::Create(createInfo, pixels);

        if (floatComponents)
            delete[] (float*)pixels;
        else
            delete[] (unsigned char*)pixels;

        //m_Data = pixels;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void TextureAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Texture.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Valid = false;
        m_Loaded = false;
    }
}