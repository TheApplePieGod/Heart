#include "hepch.h"
#include "EventEmitter.h"

namespace Heart
{
    using EventCallbackFunction = std::function<void(Event&)>;

    void EventEmitter::Subscribe(EventListener* listener)
    {
        std::lock_guard lock(m_Lock);
        HE_ENGINE_ASSERT(m_Subscribers.find(listener) == m_Subscribers.end(), "Listener is already subscribed to emitter");
        m_Subscribers.insert(listener);
    }

    void EventEmitter::Unsubscribe(EventListener* listener)
    {
        std::lock_guard lock(m_Lock);
        HE_ENGINE_ASSERT(m_Subscribers.find(listener) != m_Subscribers.end(), "Listener is unsubscribing to emitter but not subscribed");
        m_Subscribers.erase(listener);
    }

    bool EventEmitter::IsSubscribed(EventListener* listener)
    {
        std::lock_guard lock(m_Lock);
        return m_Subscribers.find(listener) != m_Subscribers.end();
    }

    void EventEmitter::Emit(Event& event)
    {
        std::lock_guard lock(m_Lock);
        for (auto listener : m_Subscribers)
            listener->OnEvent(event);
    }
}
