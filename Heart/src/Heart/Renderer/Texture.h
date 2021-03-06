#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    enum class ColorFormat
    {
        None = 0,
        RGBA8,
        R16F, RGBA16F,
        R32F, RGBA32F
    };

    enum class SamplerFilter
    {
        None = 0,
        Linear, Nearest
    };

    enum class SamplerWrapMode
    {
        None = 0,
        ClampToBorder,
        ClampToEdge,
        Repeat,
        MirroredRepeat
    };

    enum class SamplerReductionMode
    {
        None = 0,
        WeightedAverage,
        Min,
        Max
    };

    struct TextureSamplerState
    {
        SamplerFilter MinFilter = SamplerFilter::Linear;
        SamplerFilter MagFilter = SamplerFilter::Linear;
        std::array<SamplerWrapMode, 3> UVWWrap = { SamplerWrapMode::Repeat, SamplerWrapMode::Repeat, SamplerWrapMode::Repeat };
        SamplerReductionMode ReductionMode = SamplerReductionMode::WeightedAverage;
        bool AnisotropyEnable = true;
        u32 MaxAnisotropy = 8;
    };
    
    static u32 ColorFormatComponents(ColorFormat format)
    {
        switch (format)
        {
            case ColorFormat::RGBA8: return 4;
            case ColorFormat::R16F: return 1;
            case ColorFormat::RGBA16F: return 4;
            case ColorFormat::R32F: return 1;
            case ColorFormat::RGBA32F: return 4;
        }

        HE_ENGINE_ASSERT(false, "ColorFormatComponents unsupported ColorFormat");
        return 0;
    }

    static BufferDataType ColorFormatBufferDataType(ColorFormat format)
    {
        switch (format)
        {
            case ColorFormat::RGBA8: return BufferDataType::UInt8;
            case ColorFormat::R16F: return BufferDataType::HalfFloat;
            case ColorFormat::RGBA16F: return BufferDataType::HalfFloat;
            case ColorFormat::R32F: return BufferDataType::Float;
            case ColorFormat::RGBA32F: return BufferDataType::Float;
        }

        HE_ENGINE_ASSERT(false, "ColorFormatBufferDataType unsupported ColorFormat");
        return BufferDataType::None;
    }

    static ColorFormat BufferDataTypeColorFormat(BufferDataType type, u32 channelCount)
    {
        HE_ENGINE_ASSERT(channelCount == 1 || channelCount == 4, "Channel counts of 2 or 3 are not supported");
        if (channelCount == 1)
        {
            switch (type)
            {
                //case BufferDataType::UInt8: return ColorFormat::;
                case BufferDataType::Float: return ColorFormat::R32F;
                case BufferDataType::HalfFloat: return ColorFormat::R16F;
            }
        }
        else
        {
            switch (type)
            {
                case BufferDataType::UInt8: return ColorFormat::RGBA8;
                case BufferDataType::Float: return ColorFormat::RGBA32F;
                case BufferDataType::HalfFloat: return ColorFormat::RGBA16F;
            }
        }

        HE_ENGINE_ASSERT(false, "BufferDataTypeColorFormat unsupported BufferDataType and ChannelCount combination");
        return ColorFormat::None;
    }

    struct TextureCreateInfo
    {
        u32 Width, Height, Channels;
        BufferDataType DataType;
        BufferUsageType UsageType;
        u32 ArrayCount;
        u32 MipCount; // set to zero to deduce mip count
        bool AllowCPURead = false;
        TextureSamplerState SamplerState;
    };

    class Framebuffer;
    class Texture
    {
    public:
        Texture(const TextureCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~Texture() = default;

        virtual void RegenerateMipMaps() = 0;
        virtual void RegenerateMipMapsSync(Framebuffer* buffer) = 0; // sync with framebuffer drawing

        // texture must be created with 'AllowCPURead' enabled
        virtual void* GetPixelData() = 0;

        template<typename T>
        T ReadPixel(u32 x, u32 y, u32 component)
        {
            T* data = (T*)GetPixelData();
            u32 index = m_Info.Channels * (y * m_Info.Width + x);
            return data[index + component];
        }

        virtual void* GetImGuiHandle(u32 layerIndex = 0, u32 mipLevel = 0) = 0;
        
        inline u32 GetArrayCount() const { return m_Info.ArrayCount; }
        inline u32 GetWidth() const { return m_Info.Width; }
        inline u32 GetHeight() const { return m_Info.Height; }
        inline u32 GetMipCount() const { return m_Info.MipCount; }
        inline u32 GetMipWidth(u32 mipLevel) const { return std::max(static_cast<u32>(m_Info.Width * pow(0.5f, mipLevel)), 0U); }
        inline u32 GetMipHeight(u32 mipLevel) const { return std::max(static_cast<u32>(m_Info.Height * pow(0.5f, mipLevel)), 0U); }
        inline u32 GetChannels() const { return m_Info.Channels; }
        inline bool CanCPURead() const { return m_Info.AllowCPURead; }
        inline bool HasTransparency() const { return m_HasTransparency; }
        inline bool HasTranslucency() const { return m_HasTranslucency; }
        inline const TextureSamplerState& GetSamplerState() const { return m_Info.SamplerState; }

    public:
        static Ref<Texture> Create(const TextureCreateInfo& createInfo, void* initialData = nullptr);

    protected:
        void ScanForTransparency(int width, int height, int channels, void* data);

    protected:
        TextureCreateInfo m_Info;
        u32 m_MipLevels;
        bool m_HasTransparency = false;
        bool m_HasTranslucency = false;
    };
}