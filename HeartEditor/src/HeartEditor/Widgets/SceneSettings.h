#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class SceneSettings : public Widget
    {
    public:
        SceneSettings(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        ImGuiTextFilter m_EnvMapTextFilter;
    };
}
}