#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    class KeyEvent : public Event
    {
    public:
        KeyEvent(int keyCode)
            : Event(EventType::Key), m_KeyCode(keyCode)
        {}
        KeyEvent(EventType type, int keyCode)
            : Event(type), m_KeyCode(keyCode)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::Key; }

    protected:
        int m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(int key, bool repeat)
            : KeyEvent(EventType::KeyPressed, key), m_Repeat(repeat)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::WindowResize; }

    private:
        bool m_Repeat;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(int key)
            : KeyEvent(EventType::KeyReleased, key)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::KeyReleased; }
    };
}