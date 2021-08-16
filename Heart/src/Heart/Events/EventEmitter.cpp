#include "htpch.h"
#include "EventEmitter.h"

namespace Heart
{
    using EventCallbackFunction = std::function<void(Event&)>;

    void EventEmitter::Subscribe(EventListener* listener)
    {
        HT_ENGINE_ASSERT(m_Subscribers.find(listener) == m_Subscribers.end(), "Listener is already subscribed to emitter");
        m_Subscribers.insert(listener);
    }

    void EventEmitter::Unsubscribe(EventListener* listener)
    {
        HT_ENGINE_ASSERT(m_Subscribers.find(listener) != m_Subscribers.end(), "Listener is unsubscribing to emitter but not subscribed");
        m_Subscribers.erase(listener);
    }

    bool EventEmitter::IsSubscribed(const EventListener* listener) const
    {
        return m_Subscribers.find(const_cast<EventListener*>(listener)) != m_Subscribers.end();
    }

    void EventEmitter::Emit(Event& event)
    {
        //HT_ENGINE_LOG_TRACE("Emitting events");
        for (auto listener : m_Subscribers)
        {
            listener->OnEvent(event);
        }
    }
}