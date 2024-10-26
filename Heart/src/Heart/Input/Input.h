#pragma once

#include "Heart/Input/KeyCodes.h"
#include "Heart/Input/ButtonCodes.h"
#include "Heart/Input/AxisCodes.h"
#include "glm/vec2.hpp"

namespace Heart
{
    struct AxisState
    {
        double Value = 0.0;
        double OldValue = 0.0;
        double Delta = 0.0;
        u64 LastInteractionFrame = 0;
    };

    struct ButtonState
    {
        bool Pressed = false;
        u64 LastPressFrame = 0;
    };
    
    using KeyState = ButtonState;

    class Input
    {
    public:
        inline static glm::vec2 GetMousePosition() { return { GetAxisValue(AxisCode::MouseX), GetAxisValue(AxisCode::MouseY) }; }
        inline static glm::vec2 GetMouseDelta()    { return { GetAxisDelta(AxisCode::MouseX), GetAxisDelta(AxisCode::MouseY) }; }

        inline static const ButtonState& GetButtonState(ButtonCode button) { return GetButtonStateMut(button); }
        inline static const AxisState& GetAxisState(AxisCode axis)         { return GetAxisStateMut(axis); }
        inline static const KeyState& GetKeyState(KeyCode key)             { return GetKeyStateMut(key); }

        inline static bool   IsButtonPressed(ButtonCode button) { return GetButtonState(button).Pressed; }
        inline static bool   IsKeyPressed(KeyCode key)          { return GetKeyState(key).Pressed; }
        inline static double GetAxisValue(AxisCode axis)        { return GetAxisState(axis).Value; }
        inline static double GetAxisDelta(AxisCode axis)        { return GetAxisState(axis).Delta; }

    public:
        static void AddKeyEvent(KeyCode key, bool pressed);
        static void AddButtonEvent(ButtonCode button, bool pressed);
        static void AddAxisEvent(AxisCode axis, double value, bool accumulate, bool skipDelta = false);

        static void EndFrame();

    private:
        inline static ButtonState& GetButtonStateMut(ButtonCode button) { return m_ButtonStates[(u16)button]; }
        inline static AxisState& GetAxisStateMut(AxisCode axis)         { return m_AxisStates[(u16)axis]; }
        inline static KeyState& GetKeyStateMut(KeyCode key)             { return m_KeyStates[(u16)key]; }

    private:
        inline static std::array<KeyState, (u32)KeyCode::MAX + 1> m_KeyStates;
        inline static std::array<ButtonState, (u32)ButtonCode::MAX + 1> m_ButtonStates;
        inline static std::array<AxisState, (u32)AxisCode::MAX + 1> m_AxisStates;
        
    };
}
