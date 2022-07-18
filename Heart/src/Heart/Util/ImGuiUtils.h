#pragma once

#include "glm/vec2.hpp"
#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "imgui/imgui.h"

namespace Heart
{
    struct ImGuiUtils
    {
        static void RenderTooltip(const std::string& text);
        static void InputText(const char* id, std::string& text);

        static void AssetDropTarget(Asset::Type typeFilter, std::function<void(const std::string&)>&& dropCallback);
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
            const std::string& nullSelectionText,
            const std::string& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(UUID)>&& selectCallback
        );
        static void StringPicker(
            const std::vector<const char*> options,
            const std::string& selected,
            const std::string& nullSelectionText,
            const std::string& widgetId,
            ImGuiTextFilter& textFilter,
            std::function<void()>&& contextMenuCallback,
            std::function<void(size_t)>&& selectCallback
        );
    };
}