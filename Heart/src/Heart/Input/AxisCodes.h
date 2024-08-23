#pragma once

namespace Heart
{
    /*! @brief All of the significant axes represented as an enum. */
    enum class AxisCode : u16
    {
        MouseX = 0,
        MouseY,
        ScrollX,
        ScrollY,

        GamepadLeftX,
        GamepadLeftY,
        GamepadRightX,
        GamepadRightY,
        GamepadTriggerLeft,
        GamepadTriggerRight,

        MAX = GamepadTriggerRight
    };
}
