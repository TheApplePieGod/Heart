#pragma once

#include "Heart/Container/Container.hpp"

namespace Heart
{
    // TODO: typed variants
    class HString
    {
    public:
        enum class Encoding : u8
        {
            UTF8 = 0,
            UTF16 // TODO: utf32
        };

    public:
        HString() = default;
        ~HString() = default;

        HString(const char8* str)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str); }

        HString(const char16* str)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str); }

        HString(const std::basic_string<char8>& str)
            : m_Encoding(Encoding::UTF8)
        { Allocate<char8>(str.data(), str.length()); }

        HString(const std::basic_string<char16>& str)
            : m_Encoding(Encoding::UTF16)
        { Allocate<char16>(str.data(), str.length()); }

        u32 GetCount();

        HString Convert(Encoding encoding);
        HString ToUTF8();
        HString ToUTF16();

        // Unchecked
        inline char8* DataUTF8() { return Data<char8>(); }
        inline char16* DataUTF16() { return Data<char16>(); }
        inline char8 GetUTF8(u32 index) { return Get<char8>(index); }
        inline char16 GetUTF16(u32 index) { return Get<char16>(index); }
        inline u32 GetCountUTF8() { return m_Container.GetCount(); }
        inline u32 GetCountUTF16() { return m_Container.GetCount() * 0.5; }

        template <typename T>
        inline T* Data() { return reinterpret_cast<T*>(m_Container.Data()); }
        template <typename T>
        inline T& Get(u32 index) { return reinterpret_cast<T&>(m_Container[index]); }
        inline Encoding GetType() { return m_Encoding; }
        inline HString Clone() { return HString(m_Container.Clone()); }

        inline void operator=(const HString& other) { HE_PLACEMENT_NEW(&m_Container, Container<u8>, other.m_Container); }

    private:
        HString(const Container<u8>& container)
            : m_Container(container)
        {}

        template <typename T>
        void Allocate(const T* str, u32 len = 0)
        {
            if (!str) return;
            if (len == 0)
            {
                const T* ptr = (const T*)str;
                while (ptr[len] != '\0') len++;
            }
            HE_PLACEMENT_NEW(&m_Container, Container<u8>, (const u8*)str, (len + 1) * sizeof(T));
            reinterpret_cast<T*>(m_Container.Data())[len] = (T)'\0';
        }

    private:
        // We can store encoding here instead of in the pointer because strings
        // are immutable
        Encoding m_Encoding;
        Container<u8> m_Container alignas(8);
    };
}