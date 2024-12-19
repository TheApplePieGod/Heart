#pragma once

#include "Heart/Container/HString8.h"
#include "Heart/Core/UUID.h"

namespace HeartEditor
{
    enum class StatusElementType
    {
        Info,
        Success,
        Warn,
        Error,
        Loading
    };

    struct StatusElement
    {
        Heart::UUID Id;
        Heart::HString8 Text;
        StatusElementType Type;
        float Duration = 5000.f; // MS
    };

    class StatusBar
    {
    public:
        StatusBar() = default;

        void OnImGuiRender();

    private:

    };
}
