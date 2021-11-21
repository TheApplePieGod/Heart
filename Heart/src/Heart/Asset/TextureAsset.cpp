#include "htpch.h"
#include "TextureAsset.h"

#include "stb_image/stb_image.h"

namespace Heart
{
    void TextureAsset::Load()
    {
        if (m_Loaded) return;

        bool floatComponents = false;
        if (m_Extension == ".hdr") // environment map: use float components and flip on load
        {
            floatComponents = true;
            stbi_set_flip_vertically_on_load(true);
        }
        else
            stbi_set_flip_vertically_on_load(false);

        void* pixels = nullptr;
        if (floatComponents)
            pixels = stbi_loadf(m_AbsolutePath.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
        else
            pixels = stbi_load(m_AbsolutePath.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_AbsolutePath);
            m_Loaded = true;
            return;
        }

        TextureCreateInfo createInfo = {
            m_Width, m_Height, m_DesiredChannelCount,
            floatComponents,
            1, 0
        };
        m_Texture = Texture::Create(createInfo, pixels);

        m_Data = pixels;
        m_Valid = true;
        m_Loaded = true;
    }

    void TextureAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Texture.reset();
        delete[] m_Data;
        m_Data = nullptr;
        m_Valid = false;
        m_Loaded = false;
    }
}