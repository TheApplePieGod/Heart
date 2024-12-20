#pragma once

#include "Heart/Container/HString8.h"
#include "Heart/Container/HString16.h"

namespace Heart
{
    class HStringView;
    class HString
    {
    public:
        enum class Encoding : byte
        {
            UTF8 = 0,
            UTF16 // TODO: utf32
        };

    public:
        HString() = default;
        ~HString() = default;

        HString(Encoding encoding)
            : m_Encoding(encoding)
        {}

        HString(const HString& other)
            : m_Encoding(other.m_Encoding), m_Container(other.m_Container, true)
        {}

        HString(const char8* str)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str, StringUtils::StrLen(str)); }

        HString(const char16* str)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str, StringUtils::StrLen(str)); }

        HString(const char8* str, u32 len)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str, len); }

        HString(const char16* str, u32 len)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str, len); }

        HString(const char8** strs, u32* lens, u32 count)
            : m_Encoding(Encoding::UTF8)
        { AllocateMany<char8>(strs, lens, count); }

        HString(const char16** strs, u32* lens, u32 count)
            : m_Encoding(Encoding::UTF16)
        { AllocateMany<char16>(strs, lens, count); }

        HString(const std::basic_string<char8>& str)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str.data(), str.length()); }
        
        HString(const std::basic_string<char16>& str)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str.data(), str.length()); }

        explicit HString(const HStringView& other);

        u32 Count() const;
        HString Convert(Encoding encoding) const;
        HString8 ToUTF8() const;
        HString16 ToUTF16() const;
        int Compare(StringComparison type, const HStringView& other) const;
        u32 Find(const HStringView& value) const;
        HString Substr(u32 start, u32 offset = InvalidIndex);

        // Unchecked
        inline const void* DataRaw() const { return Data<void>(); }
        inline const char8* DataUTF8() const { return Data<char8>(); }
        inline const char16* DataUTF16() const { return Data<char16>(); }
        inline char8 GetUTF8(u32 index) const { return Get<char8>(index); }
        inline char16 GetUTF16(u32 index) const { return Get<char16>(index); }
        inline const char8* BeginUTF8() const { return Begin<char8>(); }
        inline const char8* EndUTF8() const { return End<char8>(); }
        inline const char16* BeginUTF16() const { return Begin<char16>(); }
        inline const char16* EndUTF16() const { return End<char16>(); }
        inline u32 CountUTF8() const { return m_Container.Data() ? (m_Container.CountUnchecked() - 1) : 0; }
        inline u32 CountUTF16() const { return m_Container.Data() ? (m_Container.CountUnchecked() * 0.5 - 1) : 0; }
        inline HStringView8 GetViewUTF8() const { return HStringView8(DataUTF8(), CountUTF8()); }
        inline HStringView16 GetViewUTF16() const { return HStringView16(DataUTF16(), CountUTF16()); }

        template <typename T>
        inline const T* Data() const
        { return !reinterpret_cast<const T*>(m_Container.Data()) ? (const T*)"" : reinterpret_cast<const T*>(m_Container.Data()); }
        template <typename T>
        inline const T& Get(u32 index) const { return reinterpret_cast<const T&>(m_Container[index]); }
        template <typename T>
        inline const T* Begin() const { return reinterpret_cast<const T*>(m_Container.Begin()); }
        template <typename T>
        inline const T* End() const { return reinterpret_cast<const T*>(m_Container.End()); }
        inline Encoding GetEncoding() const { return m_Encoding; }
        inline HString Clone() const { return HString(m_Container.Clone()); }
        inline bool IsEmpty() const { return m_Container.IsEmpty(); }
        inline void Clear() { m_Container = Container<u8>(); }

        bool operator==(const HStringView& other) const;
        bool operator<(const HStringView& other) const;
        bool operator!=(const HStringView& other) const;
        bool operator<=(const HStringView& other) const;
        bool operator>(const HStringView& other) const;
        bool operator>=(const HStringView& other) const;
        void operator=(const char8* other);
        void operator=(const char16* other);
        void operator=(const HStringView& other);
        void operator=(const HString& other);
        HString operator+(const HStringView& other) const;
        void operator+=(const HStringView& other);
    
        inline static constexpr u32 InvalidIndex = StringUtils::InvalidIndex;

    private:
        HString(const Container<u8>& container)
            : m_Container(container)
        {}

        template <typename T>
        void Allocate(const T* str, u32 len)
        {
            if (!str) return;
            if (len == 0)
            {
                m_Container.Clear(true);
                return;
            }
            m_Container.Copy(Container<u8>((const u8*)str, (len + 1) * sizeof(T)), true);
            reinterpret_cast<T*>(m_Container.Data())[len] = (T)'\0';
        }

        template <typename T>
        void AllocateMany(const T** strs, u32* lens, u32 count)
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
            m_Container.Copy(Container<u8>((totalLen + 1) * sizeof(T), true), true);
            reinterpret_cast<T*>(m_Container.Data())[totalLen] = (T)'\0';

            // Fill
            u32 dataIndex = 0;
            for (u32 i = 0; i < count; i++)
                for (u32 j = 0; j < lens[i]; j++)
                    reinterpret_cast<T*>(m_Container.Data())[dataIndex++] = strs[i][j];
        }

        template <typename T>
        HString AddPtr(const T* other, bool prepend) const
        {
            const T* data[2] = { Data<T>(), other };
            u32 lens[2] = { Count(), 0 };
            if (prepend)
            {
                data[0] = other;
                data[1] = Data<T>();
                lens[0] = 0;
                lens[1] = Count();
            }

            return HString(data, lens, 2);
        }

    private:
        // We can store encoding here instead of in the pointer because strings
        // are immutable
        Encoding m_Encoding = Encoding::UTF8;
        Container<u8> m_Container alignas(8);

        friend class HStringView;
    };

    HString operator+(const HStringView& left, const HStringView& right);

    class HStringView
    {
    public:
        HStringView() = default;
        ~HStringView() = default;

        HStringView(const HString& other)
            : m_Encoding(other.m_Encoding), m_Data(other.DataRaw()), m_Count(other.Count())
        {}

        HStringView(const std::basic_string<char8>& str)
            : m_Encoding(HString::Encoding::UTF8), m_Data(str.data()), m_Count(str.length())
        {}

        HStringView(const std::basic_string<char16>& str)
            : m_Encoding(HString::Encoding::UTF16), m_Data(str.data()), m_Count(str.length())
        {}

        constexpr HStringView(const HStringView& other)
            : m_Encoding(other.m_Encoding), m_Data(other.m_Data), m_Count(other.m_Count)
        {}

        constexpr HStringView(const HStringView8& other)
            : m_Encoding(HString::Encoding::UTF8), m_Data(other.m_Data), m_Count(other.m_Count)
        {}

        constexpr HStringView(const HStringView16& other)
            : m_Encoding(HString::Encoding::UTF16), m_Data(other.m_Data), m_Count(other.m_Count)
        {}

        constexpr HStringView(const std::basic_string_view<char8>& str)
            : m_Encoding(HString::Encoding::UTF8), m_Data(str.data()), m_Count(str.length())
        {}

        constexpr HStringView(const std::basic_string_view<char16>& str)
            : m_Encoding(HString::Encoding::UTF16), m_Data(str.data()), m_Count(str.length())
        {}

        constexpr HStringView(const char8* str)
            : m_Encoding(HString::Encoding::UTF8), m_Data(str), m_Count(StringUtils::StrLen(str))
        {}

        constexpr HStringView(const char16* str)
            : m_Encoding(HString::Encoding::UTF16), m_Data(str), m_Count(StringUtils::StrLen(str))
        {}

        constexpr HStringView(const char8* str, u32 len)
            : m_Encoding(HString::Encoding::UTF8), m_Data(str), m_Count(len)
        {}

        constexpr HStringView(const char16* str, u32 len)
            : m_Encoding(HString::Encoding::UTF16), m_Data(str), m_Count(len)
        {}

        constexpr int Compare(StringComparison type, const HStringView& other) const;
        HString Convert(HString::Encoding encoding) const;

        inline HString8 ToUTF8() const { return HString8(DataUTF8(), CountUTF8()); }
        inline HString16 ToUTF16() const { return HString16(DataUTF16(), CountUTF16()); }

        template <typename T>
        inline constexpr const T* Data() const
        { return !reinterpret_cast<const T*>(m_Data) ? (const T*)"" : reinterpret_cast<const T*>(m_Data); }
        template <typename T>
        inline constexpr const T& Get(u32 index) const { return reinterpret_cast<const T*>(m_Data)[index]; }
        template <typename T>
        inline constexpr const T* Begin() const { return reinterpret_cast<const T*>(m_Data); }
        template <typename T>
        inline constexpr const T* End() const { return reinterpret_cast<const T*>(m_Data) + m_Count; }

        inline constexpr const void* DataRaw() const { return Data<void>(); }
        inline constexpr const char8* DataUTF8() const { return Data<char8>(); }
        inline constexpr const char16* DataUTF16() const { return Data<char16>(); }
        inline constexpr char8 GetUTF8(u32 index) const { return Get<char8>(index); }
        inline constexpr char16 GetUTF16(u32 index) const { return Get<char16>(index); }
        inline constexpr const char8* BeginUTF8() const { return Begin<char8>(); }
        inline constexpr const char8* EndUTF8() const { return End<char8>(); }
        inline constexpr const char16* BeginUTF16() const { return Begin<char16>(); }
        inline constexpr const char16* EndUTF16() const { return End<char16>(); }
        inline constexpr u32 CountUTF8() const { return m_Count; }
        inline constexpr u32 CountUTF16() const { return m_Count; }
        
        inline constexpr HString::Encoding GetEncoding() const { return m_Encoding; }
        inline constexpr u32 Count() const { return m_Count; }
        inline constexpr bool IsEmpty() const { return m_Count == 0; }

        inline constexpr bool operator==(const HStringView& other) const
        { return Compare(StringComparison::Equality, other) == 0; }
        inline constexpr bool operator<(const HStringView& other) const
        { return Compare(StringComparison::Alphabetical, other) == -1; }
        inline constexpr bool operator!=(const HStringView& other) const { return !(*this == other); }
        inline constexpr bool operator<=(const HStringView& other) const { return !(*this > other); }
        inline constexpr bool operator>(const HStringView& other) const { return other < *this; }
        inline constexpr bool operator>=(const HStringView& other) const { return !(other > *this); }

    private:
        HString::Encoding m_Encoding = HString::Encoding::UTF8;
        const void* m_Data = nullptr;
        u32 m_Count = 0;

        friend class HString;
    };

    void to_json(nlohmann::json& j, const HString& str);
    void from_json(const nlohmann::json& j, HString& str);

    constexpr int HStringView::Compare(StringComparison type, const HStringView& other) const
    {
        if (other.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to compare two HStringViews with different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return 0;
        }

        switch (type)
        {
            case StringComparison::Value:
            {
                switch (m_Encoding)
                {
                    case HString::Encoding::UTF8:
                    { return StringUtils::CompareByValue(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case HString::Encoding::UTF16:
                    { return StringUtils::CompareByValue(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
            case StringComparison::Alphabetical:
            {
                switch (m_Encoding)
                {
                    case HString::Encoding::UTF8:
                    { return StringUtils::CompareAlphabetical(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case HString::Encoding::UTF16:
                    { return StringUtils::CompareAlphabetical(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
            case StringComparison::Equality:
            {
                switch (m_Encoding)
                {
                    case HString::Encoding::UTF8:
                    { return !StringUtils::CompareEq(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case HString::Encoding::UTF16:
                    { return !StringUtils::CompareEq(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
        }

        HE_ENGINE_ASSERT(false, "HStringView comparison not fully implemented");
        return 0;
    }
}

// Implement hash functionality for HString
// TODO: this is mid and we really need our own stuff going on
namespace std
{
    template<>
    struct hash<Heart::HString>
    {
        std::size_t operator()(const Heart::HString& str) const
        {
            switch (str.GetEncoding())
            {
                case Heart::HString::Encoding::UTF8:
                {
                    return std::hash<std::basic_string_view<char8>>{}(
                        std::basic_string_view<char8>(str.DataUTF8(), str.CountUTF8())
                    );
                }
                case Heart::HString::Encoding::UTF16:
                {
                    return std::hash<std::basic_string_view<char16>>{}(
                        std::basic_string_view<char16>(str.DataUTF16(), str.CountUTF16())
                    );
                }
            }

            HE_ENGINE_ASSERT(false, "HString hash function not fully implemented");
            return 0;
        }
    };
}
