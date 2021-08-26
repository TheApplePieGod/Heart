#pragma once

#include "Heart/Events/Event.h"
#include "Heart/Input/KeyCodes.h"

namespace Heart
{
    class KeyEvent : public Event
    {
    public:
        KeyEvent(KeyCode keyCode)
            : Event(EventType::Key), m_KeyCode(keyCode)
        {}
        KeyEvent(EventType type, KeyCode keyCode)
            : Event(type), m_KeyCode(keyCode)
        {}

        inline KeyCode GetKeyCode() const { return m_KeyCode; }

    public:
        static inline EventType GetStaticType() { return EventType::Key; }

    protected:
        KeyCode m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode key, bool repeat)
            : KeyEvent(EventType::KeyPressed, key), m_Repeat(repeat)
        {}

        inline bool IsRepeat() const { return m_Repeat; }

    public:
        static inline EventType GetStaticType() { return EventType::KeyPressed; }

    private:
        bool m_Repeat;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode key)
            : KeyEvent(EventType::KeyReleased, key)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::KeyReleased; }
    };
}