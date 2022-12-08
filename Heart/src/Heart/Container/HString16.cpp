#include "hepch.h"
#include "HString16.h"

#include "Heart/Container/HString.h"
#include "ww898/utf_converters.hpp"

namespace Heart
{
    HString16::HString16(const HStringView16& other)
        : HStringTyped(other)
    {}
    
    HString HString16::ToHString() const
    {
        return HString(Data(), Count());
    }

    HString8 HString16::ToUTF8() const
    {
        return HString8(ww898::utf::convz<char8>(Data()));
    }

    HString8 HStringView16::ToUTF8() const
    {
        return HString8(ww898::utf::convz<char8>(Data()));
    }

    template <>
    HStringTyped<char16> operator+(const HStringViewTyped<char16>& left, const HStringViewTyped<char16>& right)
    {
        const char16* data[2] = { left.Data(), right.Data() };
        u32 lens[2] = { left.Count(), right.Count() };
        return HStringTyped<char16>(data, lens, 2);
    }

    void to_json(nlohmann::json& j, const HString16& str)
    {
        if (str.IsEmpty())
        {
            j = "";
            return;
        }
        
        j = nlohmann::json(str.ToUTF8().Data());
    }
}