#pragma once

namespace Heart
{
    /*! @brief All of the event types. */
    enum class EventType
    {
        None = 0,
        WindowResize, WindowClose,
        Key, KeyPressed, KeyReleased,
        Button, ButtonPressed, ButtonReleased
    };

    class Event
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param type The type of this event.
         */
        Event(EventType type)
            : m_Type(type)
        {}

        /*! @brief Default constructor. */
        Event() = default;
        
        /**
         * @brief Whether or not this event has been handled and should continue to propagate.
         *
         * @warning This has not yet been implemented.
         */
        bool Handled = false;

    public:
        /*! @brief Get the event type enum that this class represents. */
        inline static EventType GetStaticType() { return EventType::None; }

    public:
        /*! @brief Get the event type that this event object represents. */
        inline EventType GetType() const { return m_Type; }

        /**
         * @brief Map a specific event type to a callback function.
         *
         * When called on an event object, it will call the callback if the
         * type of this event is the same as the type of the event passed in
         * via T.
         *
         * @tparam T The event class which should be mapped to this callback.
         * @tparam F The function signature of the callback.
         * @param callback The callback function.
         * @returns True if the callback was called, false otherwise.
         */
        template<typename T, typename F>
        bool Map(const F& callback)
        {
            if (T::GetStaticType() == m_Type)
            {
                Handled |= callback(static_cast<T&>(*this));
                return true;
            }
            return false;
        }

    protected:
        EventType m_Type;
    };

    // TODO: globalize this
    using EventCallbackFunction = std::function<void(Event&)>;
}

#define HE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
