#pragma once

#include "Heart/Container/HStringTyped.hpp"
#include "nlohmann/json.hpp"

namespace Heart
{
    class HString;
    class HString16;
    class HStringView8;
    class HString8 : public HStringTyped<char8>
    {
    public:
        HString8() = default;
        ~HString8() = default;

        HString8(const HString8& other)
            : HStringTyped(other)
        {}

        HString8(const HStringTyped<char8>& other)
            : HStringTyped(other)
        {}

        HString8(const char8* str)
            : HStringTyped(str)
        {}

        HString8(const char8* str, u32 len)
            : HStringTyped(str, len)
        {}

        HString8(const char8** strs, u32* lens, u32 count)
            : HStringTyped(strs, lens, count)
        {}

        HString8(const std::basic_string<char8>& str)
            : HStringTyped(str)
        {}
        
        explicit HString8(const HStringView8& other);

        HString ToHString() const;
        HString16 ToUTF16() const;
    };

    class HStringView8 : public HStringViewTyped<char8>
    {
    public:
        HStringView8() = default;
        ~HStringView8() = default;

        HStringView8(const HString8& other)
            : HStringViewTyped(other)
        {}

        HStringView8(const std::basic_string<char8>& str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView8(const HStringView8& other)
            : HStringViewTyped(other)
        {}

        constexpr HStringView8(const HStringViewTyped<char8>& other)
            : HStringViewTyped(other)
        {}

        constexpr HStringView8(const std::basic_string_view<char8>& str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView8(const char8* str)
            : HStringViewTyped(str)
        {}

        constexpr HStringView8(const char8* str, u32 len)
            : HStringViewTyped(str, len)
        {}

        HString16 ToUTF16() const;
    };

    void to_json(nlohmann::json& j, HStringView8 str);
    void from_json(const nlohmann::json& j, HStringView8& str);
    void from_json(const nlohmann::json& j, HString8& str);
}

namespace std
{
    template<>
    struct hash<Heart::HString8>
    {
        std::size_t operator()(const Heart::HString8& str) const
        {
            return std::hash<std::basic_string_view<char8>>{}(
                std::basic_string_view<char8>(str.Data(), str.Count())
            );
        }
    };
}
