#include "hepch.h"
#include "TextureAsset.h"

#include "Heart/Asset/AssetManager.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    void TextureAsset::Load(bool async)
    {
        HE_PROFILE_FUNCTION();
        
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        bool floatComponents = false;
        if (m_Extension == ".hdr") // environment map: use float components and flip on load
        {
            floatComponents = true;
            stbi_set_flip_vertically_on_load_thread(true);
        }
        else
            stbi_set_flip_vertically_on_load_thread(false);

        void* pixels = nullptr;
        int width, height, channels;
        if (floatComponents)
            pixels = stbi_loadf(m_AbsolutePath.Data(), &width, &height, &channels, m_DesiredChannelCount);
        else
            pixels = stbi_load(m_AbsolutePath.Data(), &width, &height, &channels, m_DesiredChannelCount);
        if (pixels == nullptr)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_AbsolutePath.Data());
            m_Loaded = true;
            m_Loading = false;
            return;
        }
        
        Flourish::ColorFormat format = Flourish::ColorFormat::RGBA8_UNORM;
        if (floatComponents)
        {
            switch (m_DesiredChannelCount)
            {
                default:
                { HE_ENGINE_ASSERT(false, "Unsupported desired channel count for texture"); } break;
                case 1: { format = Flourish::ColorFormat::R32_FLOAT; } break;
                case 4: { format = Flourish::ColorFormat::RGBA32_FLOAT; } break;
            }
        }
        else
        {
            switch (m_DesiredChannelCount)
            {
                default:
                { HE_ENGINE_ASSERT(false, "Unsupported desired channel count for texture"); } break;
                case 3: { format = Flourish::ColorFormat::RGB8_UNORM; } break;
                case 4: { format = Flourish::ColorFormat::RGBA8_UNORM; } break;
            }
        }
        
        Flourish::TextureSamplerState samp;
        samp.AnisotropyEnable = true;

        Flourish::TextureCreateInfo createInfo = {
            static_cast<u32>(width),
            static_cast<u32>(height),
            format,
            Flourish::TextureUsageType::Readonly,
            Flourish::TextureWritability::Once,
            1, 0,
            samp,
            pixels,
            static_cast<u32>(width * height * m_DesiredChannelCount) * (floatComponents ? 4 : 1),
            async,
            [this, pixels, floatComponents]()
            {
                if (!AssetManager::IsInitialized()) return;
                if (floatComponents)
                    delete[] (float*)pixels;
                else
                    delete[] (unsigned char*)pixels;

                m_Loaded = true;
                m_Loading = false;
                m_Valid = true;
            }
        };
        m_Texture = Flourish::Texture::Create(createInfo);
    }

    void TextureAsset::Unload()
    {
        if (!m_Loaded) return;
        m_Loaded = false;

        m_Texture.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Valid = false;
    }
}
