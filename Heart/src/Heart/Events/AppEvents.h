#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    /*! @brief Called before the current graphics api is shutdown. */
    class AppGraphicsShutdownEvent : public Event
    {
    public:
        AppGraphicsShutdownEvent()
            : Event(EventType::AppGraphicsShutdown)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::AppGraphicsShutdown; }
    };

    /*! @brief Called after the current graphics api is initialized. */
    class AppGraphicsInitEvent : public Event
    {
    public:
        AppGraphicsInitEvent()
            : Event(EventType::AppGraphicsInit)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::AppGraphicsInit; }
    };
}