#pragma once

#include "nlohmann/json.hpp"

namespace Heart
{
    class HArray;
    class HString;
    class HStringView;
    class Variant
    {
    public:
        enum class Type : byte
        {
            None = 0,
            Bool, Int, UInt, Float,
            String, Array
        };

    public:
        Variant() = default;
        Variant(bool value);
        Variant(s8 value);
        Variant(s16 value);
        Variant(s32 value);
        Variant(s64 value);
        Variant(u8 value);
        Variant(u16 value);
        Variant(u32 value);
        Variant(u64 value);
        Variant(float value);
        Variant(double value);
        Variant(const HArray& array);
        Variant(const HStringView& str);
        Variant(const HString& str);
        Variant(const Variant& other) { Copy(other); }
        ~Variant();

        inline Type GetType() const { return m_Type; }
        inline void SetType(Type type) { m_Type = type; }
        inline bool Bool() const { return (bool)m_Data.Bool; }
        inline s64 Int() const { return m_Data.Int; }
        inline u64 UInt() const { return m_Data.UInt; }
        inline double Float() const { return m_Data.Float; }
        HArray Array() const;
        HString String() const;

        inline void operator=(const Variant& other) { Copy(other); }

    private:
        void Copy(const Variant& other);

    private:
        Type m_Type = Type::None;
        union
        {
            byte Bool; // For safety and complete parity with c# b/c bool is technically not required to be one byte
            s64 Int;
            u64 UInt;
            double Float;
            u8 Any[16]; // Generic array of bytes to store arbitrary data
        } m_Data alignas(8); // See Variant.cs for details
    };

    void to_json(nlohmann::json& j, const Variant& variant);
    void from_json(const nlohmann::json& j, Variant& variant);
}