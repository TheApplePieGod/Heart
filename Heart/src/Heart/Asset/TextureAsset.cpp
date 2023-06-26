#include "hepch.h"
#include "TextureAsset.h"

#include "Heart/Asset/AssetManager.h"
#include "stb_image/stb_image.h"
#include "tinytiffreader.h"

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
        {
            if (m_Extension == ".tif" || m_Extension == ".tiff")
                pixels = LoadTiff(width, height, channels);
            else
                pixels = LoadImage(width, height, channels);
        }

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
            Flourish::TextureUsageFlags::Readonly,
            Flourish::TextureWritability::Once,
            1, 0,
            samp,
            pixels,
            static_cast<u32>(width * height * m_DesiredChannelCount) * (floatComponents ? 4 : 1),
            async,
            [this, async, pixels, floatComponents]()
            {
                if (!AssetManager::IsInitialized()) return;
                if (floatComponents)
                    delete[] (float*)pixels;
                else
                    delete[] (unsigned char*)pixels;

                if (async)
                {
                    m_Loaded = true;
                    m_Loading = false;
                    m_Valid = true;
                }
            }
        };
        m_Texture = Flourish::Texture::Create(createInfo);
        
        if (!async)
        {
            m_Loaded = true;
            m_Loading = false;
            m_Valid = true;
        }
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

    void* TextureAsset::LoadImage(int& outWidth, int& outHeight, int& outChannels)
    {
        return stbi_load(m_AbsolutePath.Data(), &outWidth, &outHeight, &outChannels, m_DesiredChannelCount);
    }

    void* TextureAsset::LoadTiff(int& outWidth, int& outHeight, int& outChannels)
    {
        TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(m_AbsolutePath.Data()); 
        if (!tiffr)
            return nullptr;

        u16 format = TinyTIFFReader_getSampleFormat(tiffr);
        u16 samples = TinyTIFFReader_getSamplesPerPixel(tiffr);
        u16 bits = TinyTIFFReader_getBitsPerSample(tiffr, 0);
        outWidth = TinyTIFFReader_getWidth(tiffr); 
        outHeight = TinyTIFFReader_getHeight(tiffr);
        outChannels = bits / 8 * samples;

        if (bits != 8)
        {
            HE_LOG_ERROR("Cannot import TIFF image with non 8-bit channels");
            TinyTIFFReader_close(tiffr);
            return nullptr;
        }

        // Allocate mem for one channel
        u32 totalDim = outWidth * outHeight;
        u32 totalSize = totalDim * m_DesiredChannelCount;
        u8* channel = new u8[totalDim * bits / 8];
        u8* pixels = new u8[totalSize]();

        for (u16 sample = 0; sample < samples; sample++)
        {
            TinyTIFFReader_getSampleData(tiffr, channel, sample); 
            if (TinyTIFFReader_wasError(tiffr))
            {
                HE_LOG_ERROR("TIFF read error: {0}", TinyTIFFReader_getLastError(tiffr));
                delete[] channel;
                delete[] pixels;
                TinyTIFFReader_close(tiffr);
                return nullptr;
            }

            u32 channelIndex = 0;
            for (u32 i = sample; i < totalSize; i += m_DesiredChannelCount)
                pixels[i] = channel[channelIndex++];
        }

        // Fill alpha channel with default 255
        if (m_DesiredChannelCount == 4 && outChannels < 4)
            for (u32 i = 3; i < totalSize; i += m_DesiredChannelCount)
                pixels[i] = 255;

        TinyTIFFReader_close(tiffr);
        delete[] channel;
        return pixels;
    }
}
