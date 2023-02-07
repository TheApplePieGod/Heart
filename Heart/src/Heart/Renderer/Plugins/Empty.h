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

        void Resize(u32 width, u32 height) override {}

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override {}

    private:
    
    };
}