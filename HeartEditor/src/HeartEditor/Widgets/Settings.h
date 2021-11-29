#pragma once

#include "HeartEditor/Widgets/Widget.h"

namespace HeartEditor
{
namespace Widgets
{
    class Settings : public Widget
    {
    public:
        Settings(const std::string& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:

    };
}
}