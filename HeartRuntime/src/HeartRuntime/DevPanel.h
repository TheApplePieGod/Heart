#pragma once

#include "imgui/imgui.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "HeartRuntime/Viewport.h"

namespace HeartRuntime
{
    class DevPanel
    {
    public:
        void OnImGuiRender(
            Viewport* viewport,
            Heart::Scene* scene,
            Heart::SceneRenderSettings& settings
        );
        
        inline void SetOpen(bool open) { m_Open = open; }
        inline bool IsOpen() const { return m_Open; }

    private:
        bool m_Open = false;
    };
}
