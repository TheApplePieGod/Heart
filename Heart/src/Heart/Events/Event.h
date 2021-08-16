#pragma once

namespace Heart
{
    enum EventType
    {
        None = 0,
        WindowResize
    };

    class Event
    {
    public:
        bool Handled = false;

    public:
        static inline EventType GetStaticType() { return EventType::None; }

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

#define HT_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }