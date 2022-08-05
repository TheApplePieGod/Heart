#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class AssetRegistry : public Widget
    {
    public:
        AssetRegistry(const Heart::HString& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRender() override;

    private:
        void DrawFilterPopup(const char* popupName, bool focusOnOpen, std::function<void()>&& drawCallback, std::function<void()>&& clearCallback);
        void DrawTextFilter(ImGuiTextFilter& filter, const char* popupName);
        void DrawAssetTypeFilter();
        void DrawIsResourceFilter();

        bool PassAssetTypeFilter(u32 type);
        bool PassIsResourceFilter(bool isResource);

    private:
        ImGuiTextFilter m_UUIDFilter;
        ImGuiTextFilter m_PathFilter;
        u32 m_AssetTypeFilter = 0;
        int m_IsResourceFilter = 0;
    };
}
}