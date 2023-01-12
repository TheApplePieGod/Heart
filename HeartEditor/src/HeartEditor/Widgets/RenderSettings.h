#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class RenderSettings : public Widget
    {
    public:
        RenderSettings(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRenderPostSceneUpdate() override;

    private:

    };
}
}