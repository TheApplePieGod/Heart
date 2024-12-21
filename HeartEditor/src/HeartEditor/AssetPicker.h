#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Core/UUID.h"
#include "HeartEditor/Picker.h"
#include "imgui.h"

namespace HeartEditor
{
    class AssetPicker
    {
    public:
        AssetPicker() = default;

        void OnImGuiRender(
            Heart::UUID selectedAsset,
            Heart::Asset::Type filterType,
            Heart::HStringView8 selectText,
            std::function<void()>&& contextMenuCallback,
            std::function<void(Heart::UUID)>&& selectCallback
        );

    private:
        Picker m_Picker;
        ImGuiTextFilter m_NameFilter;
        ImGuiTextFilter m_PathFilter;

        Heart::HVector<Heart::UUID> m_ProcessingIds;
    };
}
