#pragma once

namespace Heart
{
    enum class EventType
    {
        None = 0,
        WindowResize, WindowClose,
        Key, KeyPressed, KeyReleased,
        MouseButton, MouseButtonPressed, MouseButtonReleased
    };

    class Event
    {
    public:
        Event() = default;
        Event(EventType type)
            : m_Type(type)
        {}
        
        bool Handled = false;

    public:
        inline static EventType GetStaticType() { return EventType::None; }

    public:
        inline EventType GetType() const { return m_Type; }

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

    using EventCallbackFunction = std::function<void(Event&)>;
}

#define HE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }