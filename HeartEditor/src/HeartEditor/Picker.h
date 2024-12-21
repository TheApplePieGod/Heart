#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "imgui.h"

namespace HeartEditor
{
    class Picker
    {
    public:
        Picker() = default;

        void OnImGuiRender(
            Heart::HStringView8 buttonText,
            Heart::HStringView8 popupName,
            u32 rowCount,
            u32 colCount,
            std::function<void()>&& renderHeader,
            std::function<void(u32)>&& renderRow,
            std::function<void()>&& contextMenuCallback,
            std::function<void(u32)>&& selectCallback
        );

    private:

    };
}
