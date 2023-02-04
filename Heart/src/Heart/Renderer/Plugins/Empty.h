#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Heart::RenderPlugins
{
    class Empty : public RenderPlugin
    {
    public:
        Empty(HStringView8 name)
            : RenderPlugin(name)
        { m_Active = false; }
        Empty(HStringView8 name, const HVector<Ref<RenderPlugin>>& dependencies)
            : RenderPlugin(name, dependencies)
        { m_Active = false; }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;

    private:
    
    };
}