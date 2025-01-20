#pragma once

#include "Heart/Events/Event.h"
#include "Heart/Input/ButtonCodes.h"

namespace Heart
{
    /*! @brief The parent class for all button events. */
    class ButtonEvent : public Event
    {
    public:
        ButtonEvent(ButtonCode ButtonCode)
            : Event(EventType::Button), m_ButtonCode(ButtonCode)
        {
        }
        ButtonEvent(EventType type, ButtonCode ButtonCode)
            : Event(type), m_ButtonCode(ButtonCode)
        {
        }

        inline ButtonCode GetButtonCode() const { return m_ButtonCode; }

    public:
        inline static EventType GetStaticType() { return EventType::Button; }

    protected:
        ButtonCode m_ButtonCode;
    };

    /*! @brief Called when a button is pressed. */
    class ButtonPressedEvent : public ButtonEvent
    {
    public:
        ButtonPressedEvent(ButtonCode button)
            : ButtonEvent(EventType::ButtonPressed, button)
        {
        }

    public:
        inline static EventType GetStaticType() { return EventType::ButtonPressed; }
    };

    /*! @brief Called when a button is released. */
    class ButtonReleasedEvent : public ButtonEvent
    {
    public:
        ButtonReleasedEvent(ButtonCode button)
            : ButtonEvent(EventType::ButtonReleased, button)
        {
        }

    public:
        inline static EventType GetStaticType() { return EventType::ButtonReleased; }
    };
}
