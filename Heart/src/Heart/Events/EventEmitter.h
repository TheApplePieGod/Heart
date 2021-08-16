#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    struct EventSubscriber
    {
        u32 Id;
        EventCallbackFunction CallbackFunction;

        EventSubscriber(u32 id, const EventCallbackFunction& callback)
            : Id(id), CallbackFunction(callback)
        {}
    };

    class EventEmitter
    {
    public:
        u32 Subscribe(const EventCallbackFunction& callback);
        void Unsubscribe(u32 subscriberId);

    protected:
        void Emit(Event& event);

    private:
        std::vector<EventSubscriber> m_Subscribers;
        u32 m_SubscriberId = 0; 

    };
}