#pragma once

#include "glm/vec2.hpp"
#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "imgui/imgui.h"

namespace Heart
{
    class HString;
    template <typename T>
    class HVector;
    struct ImGuiUtils
    {
        static void RenderTooltip(const HStringView& text);
        static bool InputText(const char* id, HString& text);

        static void AssetDropTarget(Asset::Type typeFilter, std::function<void(const HStringView&)>&& dropCallback);
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
            const HStringView& nullSelectionText,
            const HStringView& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(UUID)>&& selectCallback
        );
        static void StringPicker(
            const HVector<HString>& options,
            const HStringView& selected,
            const HStringView& nullSelectionText,
            const HStringView& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(u32)>&& selectCallback
        );
    };
}