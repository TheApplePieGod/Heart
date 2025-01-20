#pragma once

namespace Heart
{
    /*! @brief All of the significant button codes represented as an enum. */
    enum class ButtonCode : u16
    {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,

        LeftMouse = Button1,
        RightMouse = Button2,
        MiddleMouse = Button3,

        MAX = MiddleMouse
    };
}
