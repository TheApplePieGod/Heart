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

    class EventListener;
    class EventEmitter
    {
    public:
        void Subscribe(EventListener* listener);

        // DO NOT CALL THIS INSIDE THE BODY OF AN EVENT CALLBACK
        void Unsubscribe(EventListener* listener);

        bool IsSubscribed(const EventListener* listener) const;

    protected:
        void Emit(Event& event);

    private:
        std::unordered_set<EventListener*> m_Subscribers;
    };

    class EventListener
    {
    public:
        virtual void OnEvent(Event& event) = 0;

        inline void SubscribeToEmitter(EventEmitter* emitter) { emitter->Subscribe(this); }

        // DO NOT CALL THIS INSIDE THE BODY OF AN EVENT CALLBACK
        inline void UnsubscribeFromEmitter(EventEmitter* emitter) { emitter->Unsubscribe(this); }
        
        inline bool IsSubscribedToEmitter(const EventEmitter* emitter) const { return emitter->IsSubscribed(this); }

    private:

    };

}