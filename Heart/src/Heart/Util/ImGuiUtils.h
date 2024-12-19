#pragma once

#include "glm/vec2.hpp"
#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "imgui/imgui.h"

namespace Heart
{
    class HString;
    class HString8;
    class HString8View;
    template <typename T>
    class HVector;
    struct ImGuiUtils
    {
        static void RenderTooltip(const HStringView8& text);
        static bool InputText(const char* id, HString8& text);
        static bool InputText(const char* id, HString& text, bool multiline = false);
        static void DrawFilterPopup(const char* popupName, bool focusOnOpen, std::function<void()>&& drawCallback, std::function<void()>&& clearCallback);
        static bool DrawTextFilter(ImGuiTextFilter& filter, const char* popupName);
        static bool DrawStringDropdownFilter(const char* const* options, u32 optionCount, u32& selected, const char* popupName);
        static void AssetDropTarget(Asset::Type typeFilter, std::function<void(const HString8&)>&& dropCallback);
        static bool XYZSlider(HStringView8 name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step);
        static void ResizableWindowSplitter(
            glm::vec2& storedWindowSizes,
            glm::vec2 minWindowSize,
            bool isHorizontal,
            float splitterThickness,
            float windowSpacing,
            bool splitterDisable,
            std::function<void()>&& window1Contents,
            std::function<void()>&& window2Contents
        );
        static void AssetPicker(
            Asset::Type assetType,
            UUID selectedAsset,
            const HStringView8& nullSelectionText,
            const HStringView8& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(UUID)>&& selectCallback
        );
        static void StringPicker(
            const HVector<HString8>& options,
            const HStringView8& selected,
            const HStringView8& nullSelectionText,
            const HStringView8& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(u32)>&& selectCallback
        );
    };
}
