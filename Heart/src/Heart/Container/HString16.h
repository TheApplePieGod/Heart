#pragma once

#include "Heart/Container/HStringTyped.hpp"
#include "nlohmann/json.hpp"

namespace Heart
{
    class HString;
    class HString8;
    class HStringView16;
    class HString16 : public HStringTyped<char16>
    {
    public:
        HString16() = default;
        ~HString16() = default;

        HString16(const HString16& other)
            : HStringTyped(other)
        {}

        HString16(const HStringTyped<char16>& other)
            : HStringTyped(other)
        {}

        HString16(const char16* str)
            : HStringTyped(str)
        {}

        HString16(const char16* str, u32 len)
            : HStringTyped(str, len)
        {}

        HString16(const char16** strs, u32* lens, u32 count)
            : HStringTyped(strs, lens, count)
        {}

        HString16(const std::basic_string<char16>& str)
            : HStringTyped(str)
        {}
        
        HString16(const HStringView16& other);

        HString ToHString() const;
        HString8 ToUTF8() const;
    };

    class HStringView16 : public HStringViewTyped<char16>
    {
    public:
        HStringView16() = default;
        ~HStringView16() = default;

        HStringView16(const HString16& other)
            : HStringViewTyped(other)
        {}

        HStringView16(const std::basic_string<char16>& str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView16(const HStringView16& other)
            : HStringViewTyped(other)
        {}

        constexpr HStringView16(const HStringViewTyped<char16>& other)
            : HStringViewTyped(other)
        {}

        constexpr HStringView16(const std::basic_string_view<char16>& str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView16(const char16* str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView16(const char16* str, u32 len)
            : HStringViewTyped(str, len)
        {}

        HString8 ToUTF8() const;
    };

    void to_json(nlohmann::json& j, const HString16& str);
    void from_json(const nlohmann::json& j, HString16& str);
}

namespace std
{
    template<>
    struct hash<Heart::HString16>
    {
        std::size_t operator()(const Heart::HString16& str) const
        {
            return std::hash<std::basic_string_view<char16>>{}(
                std::basic_string_view<char16>(str.Data(), str.GetCount())
            );
        }
    };
}