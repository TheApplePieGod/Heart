#pragma once

#include "Heart/Container/Container.hpp"
#include "Heart/Util/StringUtils.hpp"

namespace Heart
{
    template <typename T>
    class HStringViewTyped;
    template <typename T>
    class HStringTyped
    {
    public:
        HStringTyped() = default;
        ~HStringTyped() = default;

        HStringTyped(const HStringTyped& other)
            : m_Container(other.m_Container, true)
        {}

        HStringTyped(const T* str)
        { Allocate(str, StringUtils::StrLen(str)); }

        HStringTyped(const T* str, u32 len)
        { Allocate(str, len); }

        HStringTyped(const T** strs, u32* lens, u32 count)
        { AllocateMany(strs, lens, count); }

        HStringTyped(const std::basic_string<T>& str)
        { Allocate(str.data(), str.length()); }
        
        HStringTyped(const HStringViewTyped<T>& other);

        inline u32 Find(const HStringViewTyped<T>& value) const
        { return HStringViewTyped<T>(*this).Find(value); }
        inline u32 Find(T value) const
        { return HStringViewTyped<T>(*this).Find(value); }
        inline int Compare(StringComparison type, const HStringViewTyped<T>& other) const
        { return HStringViewTyped<T>(*this).Compare(type, other); }
        inline HStringViewTyped<T> Substr(u32 start, u32 size = InvalidIndex) const
        { return HStringViewTyped<T>(*this).Substr(start, size); }
        inline HVector<HStringViewTyped<T>> Split(HStringViewTyped<T> delim) const
        { return HStringViewTyped<T>(*this).Split(delim); }

        inline u32 Count() const { return m_Container.Data() ? (m_Container.CountUnchecked() - 1) : 0; }
        inline const T* Data() const { return !m_Container.Data() ? (const T*)"" : m_Container.Data(); }
        inline const T& Get(u32 index) const { return m_Container[index]; }
        inline const T* Begin() const { return m_Container.Begin(); }
        inline const T* End() const { return m_Container.End(); }
        inline HStringTyped Clone() const { return HStringTyped(m_Container.Clone()); }
        inline bool IsEmpty() const { return m_Container.IsEmpty(); }
        inline void Clear() { m_Container = Container<T>(); }

        inline bool operator==(const HStringViewTyped<T>& other) const
        { return StringUtils::CompareEq(Data(), Count(), other.Data(), other.Count()); }
        inline bool operator<(const HStringViewTyped<T>& other) const
        { return Compare(StringComparison::Alphabetical, other) == -1; }
        inline bool operator!=(const HStringViewTyped<T>& other) const { return !(*this == other); }
        inline bool operator<=(const HStringViewTyped<T>& other) const { return !(*this > other); }
        inline bool operator>(const HStringViewTyped<T>& other) const { return other < *this; }
        inline bool operator>=(const HStringViewTyped<T>& other) const { return !(other > *this); }
        inline const T& operator[](u32 index) const { return m_Container[index]; }
        void operator=(const HStringViewTyped<T>& other);
        void operator=(const HStringTyped<T>& other);
        HStringTyped<T> operator+(const HStringViewTyped<T>& other) const;
        void operator+=(const HStringViewTyped<T>& other);

        inline static constexpr u32 InvalidIndex = StringUtils::InvalidIndex;
    
    protected:
        HStringTyped(const Container<T>& container)
            : m_Container(container)
        {}

        void Allocate(const T* str, u32 len);
        void AllocateMany(const T** strs, u32* lens, u32 count);
        HStringTyped<T> AddPtr(const T* other, bool prepend) const;

    protected:
        Container<T> m_Container;

        friend class HString;
    };

    template <typename T>
    HStringTyped<T> operator+(const HStringViewTyped<T>& left, const HStringViewTyped<T>& right);

    template <typename T>
    class HStringViewTyped
    {
    public:
        HStringViewTyped() = default;
        ~HStringViewTyped() = default;

        HStringViewTyped(const HStringTyped<T>& other)
            : m_Data(other.Data()), m_Count(other.Count())
        {}

        HStringViewTyped(const std::basic_string<T>& str)
            : m_Data(str.data()), m_Count(str.length())
        {}

        constexpr HStringViewTyped(const HStringViewTyped<T>& other)
            : m_Data(other.m_Data), m_Count(other.m_Count)
        {}

        constexpr HStringViewTyped(const std::basic_string_view<T>& str)
            : m_Data(str.data()), m_Count(str.length())
        {}

        constexpr HStringViewTyped(const T* str)
            : m_Data(str), m_Count(StringUtils::StrLen(str))
        {}

        constexpr HStringViewTyped(const T* str, u32 len)
            : m_Data(str), m_Count(len)
        {}

        constexpr u32 Find(const HStringViewTyped<T>& value) const;
        constexpr u32 Find(T value) const;
        constexpr int Compare(StringComparison type, const HStringViewTyped<T>& other) const;
        constexpr HStringViewTyped<T> Substr(u32 start, u32 size = StringUtils::InvalidIndex) const;
        HVector<HStringViewTyped<T>> Split(HStringViewTyped<T> delim) const;

        inline constexpr u32 Count() const { return m_Count; }
        inline constexpr const T* Data() const { return !m_Data ? (const T*)"" : m_Data; }
        inline constexpr const T& Get(u32 index) const { return m_Data[index]; }
        inline constexpr const T* Begin() const { return m_Data; }
        inline constexpr const T* End() const { return m_Data + m_Count; }
        inline constexpr bool IsEmpty() const { return m_Count == 0; }

        inline constexpr bool operator==(const HStringViewTyped<T>& other) const
        { return Compare(StringComparison::Equality, other) == 0; }
        inline constexpr bool operator<(const HStringViewTyped<T>& other) const
        { return Compare(StringComparison::Alphabetical, other) == -1; }
        inline constexpr bool operator!=(const HStringViewTyped<T>& other) const { return !(*this == other); }
        inline constexpr bool operator<=(const HStringViewTyped<T>& other) const { return !(*this > other); }
        inline constexpr bool operator>(const HStringViewTyped<T>& other) const { return other < *this; }
        inline constexpr bool operator>=(const HStringViewTyped<T>& other) const { return !(other > *this); }
        inline constexpr const T& operator[](u32 index) const { return m_Data[index]; }

    protected:
        const T* m_Data;
        u32 m_Count;

        friend class HStringView;
    };

    template <typename T>
    HStringTyped<T>::HStringTyped(const HStringViewTyped<T>& other)
    {
        Allocate(other.Data(), other.Count());
    }

    template <typename T>
    void HStringTyped<T>::operator=(const HStringViewTyped<T>& other)
    {
        Allocate(other.Data(), other.Count());
    }

    template <typename T>
    void HStringTyped<T>::operator=(const HStringTyped<T>& other)
    {
        m_Container.Copy(other.m_Container, true);
    }

    template <typename T>
    HStringTyped<T> HStringTyped<T>::operator+(const HStringViewTyped<T>& other) const
    {
        const T* data[2] = { Data(), other.Data() };
        u32 lens[2] = { Count(), other.Count() };
        return HStringTyped<T>(data, lens, 2);
    }

    template <typename T>
    void HStringTyped<T>::operator+=(const HStringViewTyped<T>& other)
    {
        *this = *this + other;
    }

    template <typename T>
    void HStringTyped<T>::Allocate(const T* str, u32 len)
    {
        if (!str) return;
        if (len == 0)
        {
            m_Container.Clear(true);
            return;
        }
        m_Container.Copy(Container<T>(str, len + 1), true);
        m_Container.Data()[len] = (T)'\0';
    }

    template <typename T>
    void HStringTyped<T>::AllocateMany(const T** strs, u32* lens, u32 count)
    {
        if (!strs || !lens || count == 0) return;

        // Accumulate total length
        u32 totalLen = 0;
        for (u32 i = 0; i < count; i++)
        {
            if (lens[i] == 0)
                lens[i] = StringUtils::StrLen(strs[i]);
            totalLen += lens[i];
        }

        // Allocate & place termination char
        m_Container.Copy(Container<T>(totalLen + 1, true), true);
        m_Container.Data()[totalLen] = (T)'\0';

        // Fill
        u32 dataIndex = 0;
        for (u32 i = 0; i < count; i++)
            for (u32 j = 0; j < lens[i]; j++)
                m_Container.Data()[dataIndex++] = strs[i][j];
    }

    template <typename T>
    HStringTyped<T> HStringTyped<T>::AddPtr(const T* other, bool prepend) const
    {
        const T* data[2] = { Data(), other };
        u32 lens[2] = { Count(), 0 };
        if (prepend)
        {
            data[0] = other;
            data[1] = Data();
            lens[0] = 0;
            lens[1] = Count();
        }

        return HString(data, lens, 2);
    }

    template <typename T>
    constexpr u32 HStringViewTyped<T>::Find(const HStringViewTyped<T>& value) const
    {
        return StringUtils::Find(Data(), Count(), value.Data(), value.Count());
    }

    template <typename T>
    constexpr u32 HStringViewTyped<T>::Find(T value) const
    {
        return StringUtils::Find(Data(), Count(), &value, 1);
    }

    template <typename T>
    constexpr int HStringViewTyped<T>::Compare(StringComparison type, const HStringViewTyped<T>& other) const
    {
        switch (type)
        {
            case StringComparison::Value:
            { return StringUtils::CompareByValue(Data(), Count(), other.Data(), other.Count()); }
            case StringComparison::Alphabetical:
            { return StringUtils::CompareAlphabetical(Data(), Count(), other.Data(), other.Count()); }
            case StringComparison::Equality:
            { return !StringUtils::CompareEq(Data(), Count(), other.Data(), other.Count()); }
        }

        HE_ENGINE_ASSERT(false, "HStringViewTyped comparison not fully implemented");
        return 0;
    }

    template <typename T>
    constexpr HStringViewTyped<T> HStringViewTyped<T>::Substr(u32 start, u32 size) const
    {
        if (Count() == 0) return HStringViewTyped<T>();

        size = std::min(size, Count() - start);
        return HStringViewTyped<T>(Data() + start, size);
    }

    template <typename T>
    HVector<HStringViewTyped<T>> HStringViewTyped<T>::Split(HStringViewTyped<T> delim) const
    {
        HVector<HStringViewTyped<T>> result;
        auto indices = StringUtils::Split(Data(), Count(), delim.Data(), delim.Count());
        for (u32 i = 0; i < indices.Count(); i += 2)
            result.AddInPlace(Data() + indices[i], indices[i + 1]);
        return result;
    }
}
