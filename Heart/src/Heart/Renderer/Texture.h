#pragma once

#include "Heart/Renderer/Buffer.h"

namespace Heart
{
    enum class ColorFormat
    {
        RGBA8 = 0,
        R32F, RG32F, RGB32F, RGBA32F
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
        Texture(const std::string& path, bool floatComponents, int width, int height, int channels)
            : m_Path(path), m_FloatComponents(floatComponents), m_Width(width), m_Height(height), m_Channels(channels)
        {}
        virtual ~Texture() = default;

        inline void* GetImGuiHandle() const { return m_ImGuiHandle; }
        inline u32 GetArrayCount() const { return m_ArrayCount; }
        inline int GetWidth() const { return m_Width; }
        inline int GetHeight() const { return m_Height; }
        inline int GetChannels() const { return m_Channels; }
        inline const std::string& GetFilePath() const { return m_Path; }
        inline bool HasTransparency() const { return m_HasTransparency; }
        inline const TextureSamplerState& GetSamplerState() const { return m_SamplerState; }

    public:
        static Ref<Texture> Create(const std::string& path, bool floatComponents = false, int width = 0, int height = 0, int channels = 0, void* data = nullptr);

    protected:
        void ScanForTransparency(int width, int height, int channels, void* data);

    protected:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        std::string m_Path;
        int m_Width, m_Height, m_Channels;
        bool m_FloatComponents;
        u32 m_ArrayCount = 1;
        void* m_ImGuiHandle;
        bool m_HasTransparency = false;
        TextureSamplerState m_SamplerState;
    };
}