#pragma once

namespace Heart
{
    class HArray;
    class HString;
    class Variant
    {
    public:
        enum class Type : u8
        {
            None = 0,
            Bool, Int, Float,
            String, Array
        };

    public:
        Variant() = default;
        Variant(bool value);
        Variant(int value);
        Variant(float value);
        Variant(const HArray& array);
        Variant(const HString& str);
        Variant(const Variant& other);
        ~Variant();

        inline Type GetType() const { return m_Type; }
        inline void SetType(Type type) { m_Type = type; }
        inline bool Bool() const { return (bool)m_Data.Bool; }
        inline int Int() const { return m_Data.Int; }
        inline float Float() const { return m_Data.Float; }
        HArray Array() const;
        HString String() const;

    private:
        Type m_Type;
        union
        {
            byte Bool; // For safety and complete parity with c# b/c bool is technically not required to be one byte
            int Int;
            float Float;
            u8 Any[16]; // Generic array of bytes to store arbitrary data
        } m_Data alignas(8); // See Variant.cs for details
    };
}