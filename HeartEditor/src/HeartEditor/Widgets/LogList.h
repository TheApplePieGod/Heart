#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class LogList : public Widget
    {
    public:
        LogList(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        bool PassLevelFilter(u32 level);

    private:
        ImGuiTextFilter m_MessageFilter;
        ImGuiTextFilter m_TimestampFilter;
        ImGuiTextFilter m_SourceFilter;
        u32 m_LevelFilter = 2;
    };
}
}