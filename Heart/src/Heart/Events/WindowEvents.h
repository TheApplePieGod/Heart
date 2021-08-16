#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(u32 width, u32 height)
            : m_Width(width), m_Height(height)
        { m_Type = EventType::WindowResize; }

    public:
        static inline EventType GetStaticType() { return EventType::WindowResize; }

    private:
        u32 m_Width, m_Height;
    };
}