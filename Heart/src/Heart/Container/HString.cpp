#include "hepch.h"
#include "HString.h"

#include "ww898/utf_converters.hpp"

namespace Heart
{
    HString::HString(const HStringView& other)
        : m_Encoding(other.GetEncoding())
    {
        switch (m_Encoding)
        {
            case Encoding::UTF8: { Allocate<char8>(other.DataUTF8(), other.CountUTF8()); } return;
            case Encoding::UTF16: { Allocate<char16>(other.DataUTF16(), other.CountUTF16()); } return;
        }

        HE_ENGINE_ASSERT(false, "HString from HStringView constructor not fully implemented");
    }

    u32 HString::Count() const
    {
        switch (m_Encoding)
        {
            case Encoding::UTF8: return CountUTF8();
            case Encoding::UTF16: return CountUTF16();
        }

        HE_ENGINE_ASSERT(false, "HString Count() not fully implemented");
        return 0;
    }

    HString HString::Convert(Encoding encoding) const
    {
        if (encoding == m_Encoding) return Clone();

        switch (encoding)
        {
            case Encoding::UTF8:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF16: return HString(ww898::utf::convz<char8>(DataUTF16()));
                }
            }
            case Encoding::UTF16:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF16: return HString(ww898::utf::convz<char16>(DataUTF8()));
                }
            }
        }

        HE_ENGINE_ASSERT(false, "HString Convert() not fully implemented");
        return HString();
    }

    HString8 HString::ToUTF8() const
    {
        if (m_Encoding == Encoding::UTF8)
            return HString8(DataUTF8(), CountUTF8());

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF16:
            { return HString8(ww898::utf::convz<char8>(DataUTF16())); }
        }

        HE_ENGINE_ASSERT(false, "HString ToUTF8() not fully implemented");
        return HString8();
    }

    HString16 HString::ToUTF16() const
    {
        if (m_Encoding == Encoding::UTF16)
            return HString16(DataUTF16(), CountUTF16());

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return HString16(ww898::utf::convz<char16>(DataUTF8())); }
        }

        HE_ENGINE_ASSERT(false, "HString ToUTF16() not fully implemented");
        return HString16();
    }

    int HString::Compare(StringComparison type, const HStringView& other) const
    {
        if (other.m_Encoding != m_Encoding)
        {
            HE_ENGINE_LOG_ERROR("Attempting to compare two HStrings with different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return 0;
        }

        switch (type)
        {
            case StringComparison::Value:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF8:
                    { return StringUtils::CompareByValue(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case Encoding::UTF16:
                    { return StringUtils::CompareByValue(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
            case StringComparison::Alphabetical:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF8:
                    { return StringUtils::CompareAlphabetical(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case Encoding::UTF16:
                    { return StringUtils::CompareAlphabetical(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
            case StringComparison::Equality:
            {
                switch (m_Encoding)
                {
                    case Encoding::UTF8:
                    { return !StringUtils::CompareEq(DataUTF8(), CountUTF8(), other.DataUTF8(), other.CountUTF8()); }
                    case Encoding::UTF16:
                    { return !StringUtils::CompareEq(DataUTF16(), CountUTF16(), other.DataUTF16(), other.CountUTF16()); }
                }
            }
        }

        HE_ENGINE_ASSERT(false, "HString StringComparison not fully implemented");
        return 0;
    }

    u32 HString::Find(const HStringView& value) const
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
            { return StringUtils::Find(DataUTF8(), CountUTF8(), value.DataUTF8(), value.CountUTF8()); }
            case Encoding::UTF16:
            { return StringUtils::Find(DataUTF16(), CountUTF16(), value.DataUTF16(), value.CountUTF16()); }
        }

        HE_ENGINE_ASSERT(false, "HString find not fully implemented");
        return InvalidIndex;
    }

    HString HString::Substr(u32 start, u32 offset)
    {
        if (Count() == 0) return HString();

        u32 size = std::min(offset, Count()) - start;
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

    bool HString::operator==(const HStringView& other) const
    {
        return Compare(StringComparison::Equality, other) == 0;
    }

    bool HString::operator<(const HStringView& other) const
    {
        return Compare(StringComparison::Alphabetical, other) == -1;
    }

    bool HString::operator!=(const HStringView& other) const
    {
        return !(*this == other);
    }

    bool HString::operator<=(const HStringView& other) const
    {
        return !(*this > other);
    }

    bool HString::operator>(const HStringView& other) const
    {
        return other < *this;
    }

    bool HString::operator>=(const HStringView& other) const
    {
        return !(other > *this);
    }

    void HString::operator=(const char8* other)
    {
        if (m_Encoding != Encoding::UTF8)
        {
            HE_ENGINE_LOG_ERROR("Attempting to set UTF8 characters to a non-UTF8 HString, aborting");
            HE_ENGINE_ASSERT(false);
        }

        Allocate(other, StringUtils::StrLen(other));
    }

    void HString::operator=(const char16* other)
    {
        if (m_Encoding != Encoding::UTF16)
        {
            HE_ENGINE_LOG_ERROR("Attempting to set UTF16 characters to a non-UTF16 HString, aborting");
            HE_ENGINE_ASSERT(false);
        }

        Allocate(other, StringUtils::StrLen(other));
    }
    
    void HString::operator=(const HStringView& other)
    {
        m_Encoding = other.m_Encoding;

        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { Allocate(other.DataUTF8(), other.CountUTF8()); } return;
            case Encoding::UTF16:
            { Allocate(other.DataUTF16(), other.CountUTF16()); } return;
        }

        HE_ENGINE_ASSERT(false, "HString from HStringView operator= not fully implemented");
    }

    void HString::operator=(const HString& other)
    {
        m_Encoding = other.m_Encoding;
        m_Container.Copy(other.m_Container, true);
    }

    HString HString::operator+(const HStringView& other) const
    {
        return HStringView(*this) + other;
    }

    void HString::operator+=(const HStringView& other)
    {
        *this = *this + other;
    }

    HString operator+(const HStringView& left, const HStringView& right)
    {
        if (left.GetEncoding() != right.GetEncoding())
        {
            HE_ENGINE_LOG_ERROR("Attempting to add HStringViews of different encodings, aborting");
            HE_ENGINE_ASSERT(false);
            return HString();
        }

        switch (left.GetEncoding())
        {
            case HString::Encoding::UTF8:
            {
                const char8* data[2] = { left.DataUTF8(), right.DataUTF8() };
                u32 lens[2] = { left.CountUTF8(), right.CountUTF8() };
                return HString(data, lens, 2);
            }
            case HString::Encoding::UTF16:
            {
                const char16* data[2] = { left.DataUTF16(), right.DataUTF16() };
                u32 lens[2] = { left.CountUTF16(), right.CountUTF16() };
                return HString(data, lens, 2);
            }
        }

        HE_ENGINE_ASSERT(false, "HStringView operator+ not fully implemented");
        return HString();
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
            { j = nlohmann::json(str.ToUTF8().Data()); } return;
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