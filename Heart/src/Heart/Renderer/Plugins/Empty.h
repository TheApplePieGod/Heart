#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Heart::RenderPlugins
{
    class Empty : public RenderPlugin
    {
    public:
        Empty(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        { m_Active = false; }

    protected:
        void RenderInternal(const SceneRenderData& data) override {}
        void ResizeInternal() override {}

    private:
    
    };
}
