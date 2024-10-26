#include "hepch.h"
#include "TextureAsset.h"

#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "stb_image/stb_image.h"
#include "tinytiffreader.h"

namespace Heart
{
    void TextureAsset::LoadInternal()
    {
        HE_PROFILE_FUNCTION();

        LoadResult loadResult;
        if (m_Extension == ".tif" || m_Extension == ".tiff")
            loadResult = LoadTiff();
        else if (m_Extension == ".hetex")
            loadResult = LoadHeartTexture();
        else
            loadResult = LoadImage();

        if (!loadResult.Pixels)
        {
            HE_ENGINE_LOG_ERROR("Failed to load texture at path {0}", m_AbsolutePath.Data());
            return;
        }
        
        Flourish::TextureSamplerState samp;
        samp.AnisotropyEnable = true;

        Flourish::TextureCreateInfo createInfo = {
            loadResult.Width,
            loadResult.Height,
            loadResult.Format,
            Flourish::TextureUsageFlags::Readonly,
            1, loadResult.MipLevels ? loadResult.MipLevels : 6,
            samp,
            loadResult.Pixels,
            loadResult.DataSize,
            false,
            std::move(loadResult.Free)
        };
        m_Texture = Flourish::Texture::Create(createInfo);
        
        m_Loaded = true;
        m_Valid = true;
    }

    void TextureAsset::UnloadInternal()
    {
        m_Texture.reset();
        //delete[] m_Data;
        m_Data = nullptr;
    }

    bool TextureAsset::ShouldUnload()
    {
        // This is the only remaining reference
        return m_Texture.use_count() == 1;
    }

    TextureAsset::LoadResult TextureAsset::LoadHeartTexture()
    {
        LoadResult result{};

        u32 fileLength;
        u8* data = FilesystemUtils::ReadFile(m_AbsolutePath, fileLength);
        if (!data)
            return result;

        HeartTextureHeader* header = (HeartTextureHeader*)data;
        u8* pixels = data + sizeof(HeartTextureHeader);

        Flourish::ColorFormat format = Flourish::ColorFormat::None;
        if (header->DataType == 'f')
        {
            if (header->Channels == 1)
            {
                if (header->Precision == 2)
                    format = Flourish::ColorFormat::R16_FLOAT;
                else if (header->Precision == 4)
                    format = Flourish::ColorFormat::R32_FLOAT;
            }
            else if (header->Channels == 2)
            {
                if (header->Precision == 2)
                    format = Flourish::ColorFormat::RG16_FLOAT;
                else if (header->Precision == 4)
                    format = Flourish::ColorFormat::RG32_FLOAT;

            }
            else if (header->Channels == 4)
            {
                if (header->Precision == 2)
                    format = Flourish::ColorFormat::RGBA16_FLOAT;
                else if (header->Precision == 4)
                    format = Flourish::ColorFormat::RGBA32_FLOAT;
            }
        }
        else if (header->DataType == 'b')
        {
            if (header->Channels == 1)
            {
                if (header->Precision == 1)
                    format = Flourish::ColorFormat::R8_UNORM;
            }
            else if (header->Channels == 2)
            {
                if (header->Precision == 1)
                    format = Flourish::ColorFormat::RG8_UNORM;

            }
            else if (header->Channels == 4)
            {
                if (header->Precision == 1)
                    format = Flourish::ColorFormat::RGBA8_UNORM;
            }
        }

        if (format == Flourish::ColorFormat::None)
        {
            delete[] data;
            return result;
        }

        Flourish::TextureSamplerState samp;
        samp.AnisotropyEnable = true;

        result.Format = format;
        result.Pixels = pixels;
        result.DataSize = header->Width * header->Height * header->Channels * header->Precision;
        result.Width = header->Width;
        result.Height = header->Height;
        result.Channels = header->Channels;
        result.MipLevels = header->MipLevels;
        result.Free = [data]() { delete[] data; };

        return result;
    }


    TextureAsset::LoadResult TextureAsset::LoadImage()
    {
        LoadResult result{};

        u32 outFileSize = 0;
        u8* data = FilesystemUtils::ReadFile(m_AbsolutePath, outFileSize);
        if (!data)
            return result;

        void* pixels;
        int width, height, channels;
        if (m_Extension == ".hdr")
        {
            // environment map: use float components and flip on load
            stbi_set_flip_vertically_on_load_thread(true);
            pixels = stbi_loadf_from_memory(data, outFileSize, &width, &height, &channels, m_DesiredChannelCount);
            result.Format = Flourish::ColorFormat::RGBA32_FLOAT;
            result.DataSize = width * height * m_DesiredChannelCount * sizeof(float);
        }
        else
        {
            stbi_set_flip_vertically_on_load_thread(false);
            pixels = stbi_load_from_memory(data, outFileSize, &width, &height, &channels, m_DesiredChannelCount);
            result.Format = Flourish::ColorFormat::RGBA8_UNORM;
            result.DataSize = width * height * m_DesiredChannelCount * sizeof(u8);
        }

        result.Pixels = pixels;
        result.Width = width;
        result.Height = height;
        result.Channels = m_DesiredChannelCount;
        result.Free = [pixels]() { free(pixels); };

        delete[] data;

        return result;
    }

    TextureAsset::LoadResult TextureAsset::LoadTiff()
    {
        // TODO: incompatible with android

        LoadResult result{};

        TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(m_AbsolutePath.Data()); 
        if (!tiffr)
            return result;

        u16 format = TinyTIFFReader_getSampleFormat(tiffr);
        u16 samples = TinyTIFFReader_getSamplesPerPixel(tiffr);
        u16 bits = TinyTIFFReader_getBitsPerSample(tiffr, 0);
        result.Width = TinyTIFFReader_getWidth(tiffr); 
        result.Height = TinyTIFFReader_getHeight(tiffr);
        result.Channels = bits / 8 * samples;

        if (bits != 8)
        {
            HE_LOG_ERROR("Cannot import TIFF image with non 8-bit channels");
            TinyTIFFReader_close(tiffr);
            return result;
        }

        // Allocate mem for one channel
        u32 totalDim = result.Width * result.Height;
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
                return result;
            }

            u32 channelIndex = 0;
            for (u32 i = sample; i < totalSize; i += m_DesiredChannelCount)
                pixels[i] = channel[channelIndex++];
        }

        // Fill alpha channel with default 255
        if (m_DesiredChannelCount == 4 && result.Channels < 4)
            for (u32 i = 3; i < totalSize; i += m_DesiredChannelCount)
                pixels[i] = 255;

        result.Format = Flourish::ColorFormat::RGBA8_UNORM;
        result.Pixels = pixels;
        result.DataSize = result.Width * result.Height * result.Channels * sizeof(u8);
        result.Free = [pixels]() { delete[] pixels; };

        TinyTIFFReader_close(tiffr);
        delete[] channel;
        return result;
    }
}
