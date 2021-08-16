#include "htpch.h"
#include "EventEmitter.h"

namespace Heart
{
    using EventCallbackFunction = std::function<void(Event&)>;

    u32 EventEmitter::Subscribe(const EventCallbackFunction& callback)
    {
        m_SubscriberId++;

        m_Subscribers.emplace_back(m_SubscriberId, callback);

        return m_SubscriberId;
    }

    void EventEmitter::Unsubscribe(u32 subscriberId)
    {
        for (size_t i = 0; i < m_Subscribers.size(); i++)
        {
            if (m_Subscribers[i].Id == subscriberId)
            {
                // TODO: possibly change this so we aren't copying the std::function
                m_Subscribers[i] = m_Subscribers[m_Subscribers.size() - 1];
                m_Subscribers.pop_back();
            }
        }
    }

    void EventEmitter::Emit(Event& event)
    {
        //HT_ENGINE_LOG_TRACE("Emitting events");
        for (auto subscriber : m_Subscribers)
        {
            subscriber.CallbackFunction(event);
        }
    }
}