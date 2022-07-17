#pragma once

#include "HeartEditor/Widgets/Widget.h"

namespace HeartEditor
{
namespace Widgets
{
    class ProjectSettings : public Widget
    {
    public:
        ProjectSettings(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:

    };
}
}