#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    enum class ColorFormat
    {
        None = 0,
        RGBA8, R32F, RG32F, RGB32F, RGBA32F
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

    struct TextureSamplerState
    {
        SamplerFilter MinFilter = SamplerFilter::Linear;
        SamplerFilter MagFilter = SamplerFilter::Linear;
        std::array<SamplerWrapMode, 3> UVWWrap = { SamplerWrapMode::Repeat, SamplerWrapMode::Repeat, SamplerWrapMode::Repeat };
        bool AnisotropyEnable = true;
        u32 MaxAnisotropy = 8;
    };
    
    static u32 ColorFormatComponents(ColorFormat format)
    {
        switch (format)
        {
            case ColorFormat::RGBA8: return 4;
            case ColorFormat::R32F: return 1;
            case ColorFormat::RG32F: return 2;
            case ColorFormat::RGB32F: return 3;
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
            case ColorFormat::R32F: return BufferDataType::Float;
            case ColorFormat::RG32F: return BufferDataType::Float;
            case ColorFormat::RGB32F: return BufferDataType::Float;
            case ColorFormat::RGBA32F: return BufferDataType::Float;
        }

        HE_ENGINE_ASSERT(false, "ColorFormatBufferDataType unsupported ColorFormat");
        return BufferDataType::None;
    }

    class Texture
    {
    public:
        Texture(int width, int height, int channels, u32 arrayCount = 1, bool floatComponents = false)
            : m_Width(width), m_Height(height), m_Channels(channels), m_ArrayCount(arrayCount), m_FloatComponents(floatComponents)
        {}
        virtual ~Texture() = default;

        inline void* GetImGuiHandle(u32 layerIndex = 0) const { return m_LayerImGuiHandles[layerIndex]; }
        inline u32 GetArrayCount() const { return m_ArrayCount; }
        inline int GetWidth() const { return m_Width; }
        inline int GetHeight() const { return m_Height; }
        inline int GetChannels() const { return m_Channels; }
        inline bool HasTransparency() const { return m_HasTransparency; }
        inline const TextureSamplerState& GetSamplerState() const { return m_SamplerState; }

    public:
        static Ref<Texture> Create(int width, int height, int channels, void* data = nullptr, u32 arrayCount = 1, bool floatComponents = false);

    protected:
        void ScanForTransparency(int width, int height, int channels, void* data);

    protected:
        int m_Width, m_Height, m_Channels;
        bool m_FloatComponents;
        u32 m_ArrayCount;
        std::vector<void*> m_LayerImGuiHandles;
        bool m_HasTransparency = false;
        TextureSamplerState m_SamplerState;
    };
}