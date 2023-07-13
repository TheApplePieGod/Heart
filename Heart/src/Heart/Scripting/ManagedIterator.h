#pragma once

namespace Heart
{
    template <typename T>
    class ManagedIterator
    {
    public:
        using GetNextIterFn = bool (*)(T*);

    public:
        ManagedIterator(GetNextIterFn nextIterFn)
            : m_IterFunc(nextIterFn)
        {
            NextValue();
        }

        inline const T& operator*() const
        {
            return m_CurrentValue;
        }

        inline bool operator==(const ManagedIterator& other)
        {
            return m_CurrentPtr == other.m_CurrentPtr;
        }

        inline bool operator!=(const ManagedIterator& other)
        {
            return !(*this == other);
        }

        inline ManagedIterator& operator++()
        {
            NextValue();
            return *this;
        }

        inline ManagedIterator operator++(int)
        {
            ManagedIterator saved(*this);
            operator++();
            return saved;
        }

        /*
        inline ManagedIterator* begin() const { return this; }
        inline T* end() const { return nullptr; }
        */

    private:
        void NextValue()
        {
            if (m_IterFunc && m_IterFunc(&m_CurrentValue))
                m_CurrentPtr = &m_CurrentValue;
            else
                m_CurrentPtr = nullptr;
        }

    private:
        T m_CurrentValue;
        T* m_CurrentPtr = nullptr;
        GetNextIterFn m_IterFunc = nullptr;
    };
}
