#pragma once

#include "glm/mat4x4.hpp"
#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"

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
            float Radius;
            glm::vec2 Padding;
        };

    public:
        LightingData(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        { Initialize(); }

        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }
        inline const Flourish::AccelerationStructure* GetLightTLAS() const { return m_LightTLAS.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        Ref<Flourish::Buffer> m_Buffer;
        Ref<Flourish::AccelerationStructure> m_LightTLAS;
        Ref<Flourish::AccelerationStructure> m_LightBLAS;

        HVector<Flourish::AccelerationStructureInstance> m_Instances;
        HVector<glm::mat4> m_Transforms;

        u32 m_MaxLights = 750;
        bool m_UseRayTracing;
    };
}
