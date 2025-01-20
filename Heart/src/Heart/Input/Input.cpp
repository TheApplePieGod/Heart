#include "hepch.h"
#include "Input.h"

#include "Heart/Core/App.h"

namespace Heart
{
    void Input::AddKeyEvent(KeyCode key, bool pressed)
    {
        auto& state = GetKeyStateMut(key);
        state.Pressed = pressed;
        state.LastPressFrame = App::Get().GetFrameCount();
    }

    void Input::AddButtonEvent(ButtonCode button, bool pressed)
    {
        auto& state = GetButtonStateMut(button);
        state.Pressed = pressed;
        state.LastPressFrame = App::Get().GetFrameCount();
    }

    void Input::AddAxisEvent(AxisCode axis, double value, bool accumulate, bool skipDelta)
    {
        auto& state = GetAxisStateMut(axis);
        if (accumulate)
        {
            state.Value += value;
            if (!skipDelta)
                state.Delta += value;
        }
        else
        {
            state.Value = value;
            if (!skipDelta)
                state.Delta = state.Value - state.OldValue;
        }
        state.LastInteractionFrame = App::Get().GetFrameCount();
    }

    void Input::EndFrame()
    {
        // Clear deltas
        for (u32 i = 0; i < m_AxisStates.size(); i++)
        {
            auto& state = m_AxisStates[i];
            state.OldValue = state.Value;
            state.Delta = 0.0;
        }
    }
}
