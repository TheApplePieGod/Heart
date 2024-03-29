#pragma once

#include "Heart/Events/Event.h"
#include "Heart/Input/MouseCodes.h"

namespace Heart
{
    /*! @brief The parent class for all mouse button events. */
    class MouseButtonEvent : public Event
    {
    public:
        MouseButtonEvent(MouseCode mouseCode)
            : Event(EventType::MouseButton), m_MouseCode(mouseCode)
        {}
        MouseButtonEvent(EventType type, MouseCode mouseCode)
            : Event(type), m_MouseCode(mouseCode)
        {}

        inline MouseCode GetMouseCode() const { return m_MouseCode; }

    public:
        inline static EventType GetStaticType() { return EventType::MouseButton; }

    protected:
        MouseCode m_MouseCode;
    };

    /*! @brief Called when a mouse button is pressed. */
    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseCode button)
            : MouseButtonEvent(EventType::MouseButtonPressed, button)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    };

    /*! @brief Called when a mouse button is released. */
    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseCode button)
            : MouseButtonEvent(EventType::MouseButtonReleased, button)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    };
}