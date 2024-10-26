#pragma once

#include "Heart/Events/Event.h"

namespace Heart
{
    class EventListener;
    class EventEmitter
    {
    public:
        /**
         * @brief Add a listener to this emitter's list of subscribers.
         *
         * @note Duplicate entries will fail an assert.
         * @warning Do not call in the body of an event callback.
         * @todo Change assert behavior?
         *
         * @param listener The subscribing listener.
         */
        void Subscribe(EventListener* listener);

        /**
         * @brief Remove a listener from this emitter's list of subscribers.
         *
         * @note Non-existent entries will fail an assert.
         * @warning Do not call in the body of an event callback.
         * @todo Change assert behavior?
         *
         * @param listener The subscribed listener.
         */
        void Unsubscribe(EventListener* listener);

        /**
         * @brief Check if a listener is subscribed to this emitter.
         *
         * @param listener The listener to check.
         */
        bool IsSubscribed(EventListener* listener);

        /**
         * @brief Emit an event to all subscribers.
         *
         * @param event The event to emit.
         */
        void Emit(Event& event);

    private:
        std::unordered_set<EventListener*> m_Subscribers;
        std::recursive_mutex m_Lock;
    };

    class EventListener
    {
    public:
        /**
         * @brief Called when an emitter this listener is subscribed to emits an event.
         *
         * @param event The event emitted.
         */
        virtual void OnEvent(Event& event) = 0;

        /**
         * @brief Shortcut for subscribing to an emitter. Calls EventEmitter::Subscribe().
         *
         * @see EventEmitter::Subscribe
         *
         * @param emitter The emitter to subscribe to.
         */
        inline void SubscribeToEmitter(EventEmitter* emitter) { if (emitter) emitter->Subscribe(this); }

        /**
         * @brief Shortcut for unsubscribing from an emitter. Calls EventEmitter::Unsubscribe().
         *
         * @see EventEmitter::Unsubscribe
         *
         * @param emitter The emitter to unsubscribe from.
         */
        inline void UnsubscribeFromEmitter(EventEmitter* emitter) { if (emitter) emitter->Unsubscribe(this); }
        
        /**
         * @brief Shortcut for checking emitter subscription status. Calls EventEmitter::IsSubscribed().
         *
         * @see EventEmitter::IsSubscribed
         *
         * @param emitter The emitter to check the subscription status of.
         */
        inline bool IsSubscribedToEmitter(EventEmitter* emitter) { return emitter && emitter->IsSubscribed(this); }
    };

}
