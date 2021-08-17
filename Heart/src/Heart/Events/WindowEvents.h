#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(u32 width, u32 height)
            : Event(EventType::WindowResize), m_Width(width), m_Height(height)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::WindowResize; }

    private:
        u32 m_Width, m_Height;
    };

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent()
            : Event(EventType::WindowClose)
        {}

    public:
        static inline EventType GetStaticType() { return EventType::WindowClose; }
    };
}