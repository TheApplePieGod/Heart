#pragma once

#include "glm/mat4x4.hpp"
#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"

namespace Heart::RenderPlugins
{
    class TLAS : public RenderPlugin
    {
    public:
        TLAS(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        { Initialize(); }

        inline const Flourish::AccelerationStructure* GetAccelStructure() const { return m_AccelStructure.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        HVector<Flourish::AccelerationStructureInstance> m_Instances;
        Ref<Flourish::AccelerationStructure> m_AccelStructure;
    };
}
