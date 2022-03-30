#pragma once

namespace Heart
{
    /*! @brief All of the significant mouse button codes represented as an enum. */
    enum class MouseCode : u16
    {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,

        LeftButton = Button1,
        RightButton = Button2,
        MiddleButton = Button3
    };
}