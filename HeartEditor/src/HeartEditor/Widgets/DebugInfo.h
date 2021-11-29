#pragma once

#include "HeartEditor/Widgets/Widget.h"

namespace HeartEditor
{
namespace Widgets
{
    class DebugInfo : public Widget
    {
    public:
        DebugInfo(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:

    };
}
}