#pragma once

namespace Heart
{
    // Must keep the c# version of this class consistent
    class Variant
    {
    public:
        enum class Type
        {
            None = 0,
            Bool, Int, Float,
            String, Array
        };

    public:
        Variant(bool value);

    private:
        union
        {
            bool Bool;
            int Int;
            float Float;
            u8 Data[]; // Generic array of bytes to store arbitrary data
        } m_Data alignas(8);
    };
}