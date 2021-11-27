#pragma once

#include "glm/vec2.hpp"
#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "imgui/imgui.h"

namespace Heart
{
    struct ImGuiUtils
    {
        static void ResizableWindowSplitter(glm::vec2& storedWindowSizes, glm::vec2 minWindowSize, bool isHorizontal, float splitterThickness, float windowSpacing, bool splitterDisable, std::function<void()>&& window1Contents, std::function<void()>&& window2Contents);
        static void AssetPicker(Asset::Type assetType, UUID selectedAsset, const std::string& nullSelectionText, const std::string& widgetId, ImGuiTextFilter& textFilter, std::function<void()>&& contextMenuCallback, std::function<void(UUID)>&& selectCallback);
    };
}