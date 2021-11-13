#include "htpch.h"
#include "TextureAsset.h"

#include "stb_image/stb_image.h"

namespace Heart
{
    void TextureAsset::Load()
    {
        if (m_Loaded) return;

        unsigned char* pixels = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_Path);
            HE_ENGINE_ASSERT(false);
        }

        m_Texture = Texture::Create(m_Path, m_Width, m_Height, m_Channels, pixels);

        m_Data = pixels;
        m_Loaded = true;
    }

    void TextureAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Texture.reset();
        delete[] m_Data;
        m_Data = nullptr;
        m_Loaded = false;
    }
}