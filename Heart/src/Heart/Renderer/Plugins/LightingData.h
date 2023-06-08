#pragma once

#include "glm/mat4x4.hpp"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Buffer;
}

namespace Heart::RenderPlugins
{
    class LightingData : public RenderPlugin
    {
    public:
        struct LightData
        {
            glm::vec4 Position;
            glm::vec4 Direction;
            glm::vec4 Color;
            u32 LightType;
            float ConstantAttenuation;
            float LinearAttenuation;
            float QuadraticAttenuation;
        };

    public:
        LightingData(HStringView8 name)
            : RenderPlugin(name)
        { Initialize(); }

        void Resize(u32 width, u32 height) override {}
        
        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;

    private:
        void Initialize();

    private:
        Ref<Flourish::Buffer> m_Buffer;
        u32 m_MaxLights = 100;
    };
}
