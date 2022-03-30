#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    /*! @brief Called when a window is resized. */
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(u32 width, u32 height)
            : Event(EventType::WindowResize), m_Width(width), m_Height(height)
        {}
        inline u32 GetWidth() const { return m_Width; }
        inline u32 GetHeight() const { return m_Height; }

    public:
        inline static EventType GetStaticType() { return EventType::WindowResize; }

    private:
        u32 m_Width, m_Height;
    };

    /*! @brief Called when a window is closing. */
    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent()
            : Event(EventType::WindowClose)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::WindowClose; }
    };
}