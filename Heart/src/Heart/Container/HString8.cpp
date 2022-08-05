#include "hepch.h"
#include "HString8.h"

#include "Heart/Container/HString.h"
#include "ww898/utf_converters.hpp"

namespace Heart
{
    HString8::HString8(const HStringView8& other)
        : HStringTyped(other)
    {}

    HString HString8::ToHString() const
    {
        return HString(Data(), GetCount());
    }

    HString16 HString8::ToUTF16() const
    {
        return HString16(ww898::utf::convz<char16>(Data()));
    }

    HString16 HStringView8::ToUTF16() const
    {
        return HString16(ww898::utf::convz<char16>(Data()));
    }

    HStringTyped<char8> operator+(const HStringViewTyped<char8>& left, const HStringViewTyped<char8>& right)
    {
        const char8* data[2] = { left.Data(), right.Data() };
        u32 lens[2] = { left.GetCount(), right.GetCount() };
        return HStringTyped<char8>(data, lens, 2);
    }

    void to_json(nlohmann::json& j, const HString8& str)
    {
        if (str.IsEmpty())
        {
            j = "";
            return;
        }
        
        j = nlohmann::json(str.Data());
    }

    void from_json(const nlohmann::json& j, HString8& str)
    {
        if (!j.is_string()) return;
        
        str = HString8(j.get<const nlohmann::json::string_t*>()->c_str());
    }
}