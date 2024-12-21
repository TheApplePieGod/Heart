#pragma once

#include "HeartEditor/AssetPicker.h"
#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class SceneSettings : public Widget
    {
    public:
        SceneSettings(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        ImGuiTextFilter m_EnvMapTextFilter;
        AssetPicker m_AssetPicker;
    };
}
}
