#pragma once

#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class Cubemap
    {
    public:
        Cubemap(bool floatComponents, int width, int height)
            : m_FloatComponents(floatComponents), m_Width(width), m_Height(height)
        {}
        virtual ~Cubemap() = default;

        //inline void* GetImGuiHandle() const { return m_ImGuiHandle; }
        inline int GetWidth() const { return m_Width; }
        inline int GetHeight() const { return m_Height; }
        inline const TextureSamplerState& GetSamplerState() const { return m_SamplerState; }

    public:
        static Ref<Cubemap> Create(int width, int height, bool floatComponents = false);

    protected:
        const int m_DesiredChannelCount = 4; // all cubemaps will have 4 channels
        int m_Width, m_Height;
        bool m_FloatComponents;
        //void* m_ImGuiHandle;
        TextureSamplerState m_SamplerState;
    };
}