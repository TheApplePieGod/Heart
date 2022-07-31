#pragma once

#include "Heart/Container/Container.hpp"
#include "nlohmann/json.hpp"

namespace Heart
{
    // TODO: typed variants
    // TODO: macros for duplicated code?
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

        HString(const HString& other)
            : m_Encoding(other.m_Encoding), m_Container(other.m_Container)
        {}

        HString(const char8* str)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str); }

        HString(const char16* str)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str); }

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

        u32 GetCount() const;
        HString Convert(Encoding encoding) const;
        HString ToUTF8() const;
        HString ToUTF16() const;

        // Unchecked
        // TODO: checked data() call
        inline const void* DataRaw() const { return Data<void>(); }
        inline const char8* DataUTF8() const { return Data<char8>(); }
        inline const char16* DataUTF16() const { return Data<char16>(); }
        inline char8 GetUTF8(u32 index) const { return Get<char8>(index); }
        inline char16 GetUTF16(u32 index) const { return Get<char16>(index); }
        // Subtract one b/c of null character
        inline u32 GetCountUTF8() const { return m_Container.Data() ? (m_Container.GetCountUnchecked() - 1) : 0; }
        inline u32 GetCountUTF16() const { return m_Container.Data() ? (m_Container.GetCountUnchecked() * 0.5 - 1) : 0; }

        template <typename T>
        inline T* Data() const { return reinterpret_cast<T*>(m_Container.Data()); }
        template <typename T>
        inline T& Get(u32 index) const { return reinterpret_cast<T&>(m_Container[index]); }
        inline Encoding GetEncoding() const { return m_Encoding; }
        inline HString Clone() const { return HString(m_Container.Clone()); }
        inline bool Empty() const { return GetCount() == 0; }
        inline void Clear() { m_Container.Resize(0); }

        bool operator==(const HString& other) const;
        void operator=(const HString& other);
        HString operator+(const HString& other) const;
        HString operator+(const char8* other) const;
        HString operator+(const char16* other) const;

    private:
        HString(const Container<u8>& container)
            : m_Container(container)
        {}

        template <typename T>
        u32 StrLen(const T* str) const
        {
            if (!str) return 0;
            u32 len = 0;
            while (str[len] != (T)'\0') len++;
            return len;
        }

        template <typename T>
        bool Compare(const T* str1, u32 len1, const T* str2, u32 len2) const
        {
            if (!str1 || !str2) return false;
            u32 ptr = 0;
            while (ptr < len1 && ptr < len2)
            {
                if (str1[ptr] != str2[ptr]) return false;
                ptr++;
            }
            return true;
        }

        template <typename T>
        void Allocate(const T* str, u32 len = 0)
        {
            if (!str) return;
            if (len == 0) len = StrLen(str);
            HE_PLACEMENT_NEW(&m_Container, Container<u8>, (const u8*)str, (len + 1) * sizeof(T));
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
                    lens[i] = StrLen(strs[i]);
                totalLen += lens[i];
            }

            // Allocate & place termination char
            m_Container = Container<u8>((totalLen + 1) * sizeof(T), true);
            reinterpret_cast<T*>(m_Container.Data())[totalLen] = (T)'\0';

            // Fill
            u32 dataIndex = 0;
            for (u32 i = 0; i < count; i++)
                for (u32 j = 0; j < lens[i]; j++)
                    reinterpret_cast<T*>(m_Container.Data())[dataIndex++] = strs[i][j];
        }

    private:
        // We can store encoding here instead of in the pointer because strings
        // are immutable
        Encoding m_Encoding = Encoding::UTF8;
        Container<u8> m_Container alignas(8);
    };

    void to_json(nlohmann::json& j, const HString& str);
    void from_json(const nlohmann::json& j, HString& str);
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
                        std::basic_string_view<char8>(str.DataUTF8(), str.GetCountUTF8())
                    );
                }
                case Heart::HString::Encoding::UTF16:
                {
                    return std::hash<std::basic_string_view<char16>>{}(
                        std::basic_string_view<char16>(str.DataUTF16(), str.GetCountUTF16())
                    );
                }
            }

            HE_ENGINE_ASSERT(false, "HString hash function not fully implemented");
            return 0;
        }
    };
}