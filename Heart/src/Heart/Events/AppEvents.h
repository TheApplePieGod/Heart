#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    class AppGraphicsShutdownEvent : public Event
    {
    public:
        AppGraphicsShutdownEvent()
            : Event(EventType::AppGraphicsShutdown)
        {}

    public:
        inline static EventType GetStaticType() { return EventType::AppGraphicsShutdown; }
    };

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