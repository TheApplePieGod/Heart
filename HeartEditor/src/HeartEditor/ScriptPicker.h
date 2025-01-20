#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Container/HString.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Core/UUID.h"
#include "HeartEditor/Picker.h"
#include "imgui.h"

namespace HeartEditor
{
    class ScriptPicker
    {
    public:
        ScriptPicker() = default;

        void OnImGuiRender(
            s64 currentScript,
            Heart::HStringView8 selectText,
            std::function<void()>&& contextMenuCallback,
            std::function<void(s64)>&& selectCallback
        );

    private:
        Picker m_Picker;
        ImGuiTextFilter m_NameFilter;

        Heart::HVector<std::pair<s64, Heart::HStringView>> m_Processing;
    };
}
