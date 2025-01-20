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

        void OnImGuiRenderPostSceneUpdate() override;
        
    private:
        struct ListEntry
        {
            Heart::LogListEntry Entry;
            bool Partial;
        };
        
    private:
        bool PassLevelFilter(u32 level);

    private:
        const u32 m_MaxEntries = 5000;

        Heart::HVector<ListEntry> m_ProcessingEntries;
        Heart::HVector<ListEntry> m_FilteredEntries;
        ImGuiTextFilter m_MessageFilter;
        ImGuiTextFilter m_TimestampFilter;
        ImGuiTextFilter m_SourceFilter;
        float m_LastWidth = 0;
        u32 m_LevelFilter = 1;
    };
}
}
