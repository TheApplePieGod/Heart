#include "hepch.h"
#include "HString.h"

#include <ww898/utf_converters.hpp>

namespace Heart
{
    u32 HString::GetCount()
    {
        switch (m_Encoding)
        {
            case Encoding::UTF8: return GetCountUTF8();
            case Encoding::UTF16: return GetCountUTF16();
        }

        HE_ENGINE_ASSERT("HString GetCount() not fully implemented");
        return 0;
    }

    HString HString::Convert(Encoding encoding)
    {
        if (encoding == m_Encoding) return Clone();

        switch (encoding)
        {
            case Encoding::UTF8: return ToUTF8();
            case Encoding::UTF16: return ToUTF16();
        }

        HE_ENGINE_ASSERT("HString Convert() not fully implemented");
        return HString();
    }

    HString HString::ToUTF8()
    {
        if (m_Encoding == Encoding::UTF8) return Clone();

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF16:
            { return HString(ww898::utf::convz<char8>(DataUTF16())); }
        }

        HE_ENGINE_ASSERT("HString ToUTF8() not fully implemented");
        return HString();
    }

    HString HString::ToUTF16()
    {
        if (m_Encoding == Encoding::UTF16) return Clone();

        // Not optimal because the converted string is being allocated twice,
        // but it's fine for now
        switch (m_Encoding)
        {
            case Encoding::UTF8:
            { return HString(ww898::utf::convz<char16>(DataUTF8())); }
        }

        HE_ENGINE_ASSERT("HString ToUTF16() not fully implemented");
        return HString();
    }
}