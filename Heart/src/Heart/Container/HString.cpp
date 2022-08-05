#include "hepch.h"
#include "HString.h"

#include "ww898/utf_converters.hpp"

namespace Heart
{
    u32 HString::GetCount() const
    {
        switch (m_Encoding)
        {
            case Encoding::UTF8: return GetCountUTF8();
            case Encoding::UTF16: return GetCountUTF16();
        }

        HE_ENGINE_ASSERT(false, "HString GetCount() not fully implemented");
        return 0;
    }

    HString HString::Convert(Encoding encoding) const
    {
        if (encoding == m_Encoding) return Clone();

        switch (encoding)
        {
            case Encoding::UTF8: return ToUTF8();
            case Encoding::UTF16: return ToUTF16();
        }

        HE_ENGINE_ASSERT(false, "HString Convert() not fully implemented");
        return HString();
    }

    HString HString::ToUTF8() const
    {
        if (m_Encoding == Encoding::UTF8) return Clone();

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF16:
            { return HString(ww898::utf::convz<char8>(DataUTF16())); }
        }

        HE_ENGINE_ASSERT(false, "HString ToUTF8() not fully implemented");
        return HString();
    }

    HString HString::ToUTF16() const
    {
        if (m_Encoding == Encoding::UTF16) return Clone();

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return HString(ww898::utf::convz<char16>(DataUTF8())); }
        }

        HE_ENGINE_ASSERT(false, "HString ToUTF16() not fully implemented");
        return HString();
    }

    int HString::Compare(Comparison type, const HString& other) const
    {
        if (other.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to compare two HStrings with different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return 0;
        }

        switch (type)
        {
            case Comparison::Value:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF8:
                    { return CompareByValue(DataUTF8(), GetCountUTF8(), other.DataUTF8(), other.GetCountUTF8()); }
                    case Encoding::UTF16:
                    { return CompareByValue(DataUTF16(), GetCountUTF16(), other.DataUTF16(), other.GetCountUTF16()); }
                }
            }
            case Comparison::Alphabetical:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF8:
                    { return CompareAlphabetical(DataUTF8(), GetCountUTF8(), other.DataUTF8(), other.GetCountUTF8()); }
                    case Encoding::UTF16:
                    { return CompareAlphabetical(DataUTF16(), GetCountUTF16(), other.DataUTF16(), other.GetCountUTF16()); }
                }
            }
        }

        HE_ENGINE_ASSERT(false, "HString comparison not fully implemented");
        return 0;
    }

    u32 HString::Find(const HString& value) const
    {
        if (value.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to search a HString for a value with a different encoding, aborting");
            HE_ENGINE_ASSERT(false);
            return InvalidIndex;
        }

        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return Find(DataUTF8(), GetCountUTF8(), value.DataUTF8(), value.GetCountUTF8()); }
            case Encoding::UTF16:
            { return Find(DataUTF16(), GetCountUTF16(), value.DataUTF16(), value.GetCountUTF16()); }
        }

        HE_ENGINE_ASSERT(false, "HString find not fully implemented");
        return InvalidIndex;
    }

    u32 HString::FindUTF8Char(char8 value) const
    {
        if (m_Encoding != Encoding::UTF8)
        {
            HE_ENGINE_LOG_ERROR("Attempting to search a non-UTF8 HString for a UTF8 value, aborting");
            HE_ENGINE_ASSERT(false);
            return InvalidIndex;
        }

        return Find(DataUTF8(), GetCountUTF8(), &value, 1);
    }

    u32 HString::FindUTF16Char(char16 value) const
    {
        if (m_Encoding != Encoding::UTF16)
        {
            HE_ENGINE_LOG_ERROR("Attempting to search a non-UTF16 HString for a UTF16 value, aborting");
            HE_ENGINE_ASSERT(false);
            return InvalidIndex;
        }

        return Find(DataUTF16(), GetCountUTF16(), &value, 1);
    }

    HString HString::Substr(u32 start, u32 offset)
    {
        if (GetCount() == 0) return HString();

        u32 size = std::min(offset, GetCount()) - start;
        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return HString(DataUTF8() + start, size); }
            case Encoding::UTF16:
            { return HString(DataUTF16() + start, size); }
        }

        HE_ENGINE_ASSERT(false, "HString substr not fully implemented");
        return HString();   
    }

    bool HString::operator==(const HString& other) const
    {
        if (other.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to compare two HStrings with different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return false;
        }

        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return CompareEq(DataUTF8(), GetCountUTF8(), other.DataUTF8(), other.GetCountUTF8()); }
            case Encoding::UTF16:
            { return CompareEq(DataUTF16(), GetCountUTF16(), other.DataUTF16(), other.GetCountUTF16()); }
        }

        HE_ENGINE_ASSERT(false, "HString equality operator not fully implemented");
        return false;
    }

    bool HString::operator!=(const HString& other) const
    {
        return !(*this == other);
    }

    bool HString::operator<(const HString& other) const
    {
        return Compare(Comparison::Alphabetical, other) == -1;
    }

    bool HString::operator<=(const HString& other) const
    {
        return !(*this > other);
    }

    bool HString::operator>(const HString& other) const
    {
        return other < *this;
    }

    bool HString::operator>=(const HString& other) const
    {
        return !(other > *this);
    }
    
    void HString::operator=(const HString& other)
    {
        m_Encoding = other.m_Encoding;
        m_Container = other.m_Container;
    }

    HString HString::operator+(const HString& other) const
    {
        if (other.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to add two HStrings with different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        switch (m_Encoding)
        {
            case Encoding::UTF8:
            {
                const char8* data[2] = { DataUTF8(), other.DataUTF8() };
                u32 lens[2] = { GetCountUTF8(), other.GetCountUTF8() };
                return HString(data, lens, 2);
            }
            case Encoding::UTF16:
            {
                const char16* data[2] = { DataUTF16(), other.DataUTF16() };
                u32 lens[2] = { GetCountUTF16(), other.GetCountUTF16() };
                return HString(data, lens, 2);
            }
        }

        HE_ENGINE_ASSERT(false, "HString operator+ not fully implemented");
        return HString();
    }

    HString HString::operator+(const char8* other) const
    {
        if (m_Encoding != Encoding::UTF8)
        {
            HE_ENGINE_LOG_ERROR("Attempting to add UTF8 characters to a non-UTF8 HString, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        return AddPtr<char8>(other, false);
    }

    HString HString::operator+(const char16* other) const
    {
        if (m_Encoding != Encoding::UTF16)
        {
            HE_ENGINE_LOG_ERROR("Attempting to add UTF16 characters to a non-UTF16 HString, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        return AddPtr<char16>(other, false);
    }

    void HString::operator+=(const HString& other)
    {
        *this = *this + other;
    }

    void HString::operator+=(const char8* other)
    {
        *this = *this + other;
    }

    void HString::operator+=(const char16* other)
    {
        *this = *this + other;
    }

    HString operator+(const char8* left, const HString& right)
    {
        if (right.GetEncoding() != HString::Encoding::UTF8)
        {
            HE_ENGINE_LOG_ERROR("Attempting to add UTF8 characters to a non-UTF8 HString, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        return right.AddPtr<char8>(left, true);
    }

    HString operator+(const char16* left, const HString& right)
    {
        if (right.GetEncoding() != HString::Encoding::UTF16)
        {
            HE_ENGINE_LOG_ERROR("Attempting to add UTF16 characters to a non-UTF16 HString, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        return right.AddPtr<char16>(left, true);
    }

    void to_json(nlohmann::json& j, const HString& str)
    {
        if (str.IsEmpty())
        {
            j = "";
            return;
        }
        
        switch (str.GetEncoding())
        {
            default:
            { j = nlohmann::json(str.ToUTF8().DataUTF8()); } return;
            case HString::Encoding::UTF8:
            { j = nlohmann::json(str.DataUTF8()); } return;
        }
    }

    void from_json(const nlohmann::json& j, HString& str)
    {
        if (!j.is_string()) return;
        
        str = HString(j.get<const nlohmann::json::string_t*>()->c_str());
    }
}