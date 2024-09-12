#pragma once

namespace Heart
{
    struct BitUtils
    {
        // https://stackoverflow.com/questions/71539154/convert-power-of-2-bitmask-into-corresponding-index
        // Converts a power of two integer into its corresponding bit index
        // 1 -> 0, 2 -> 1, 4 -> 2, etc.
        static constexpr u32 PowerOfTwoIndex(u32 input)
        {
            input -= 1;
            input = input - ((input >> 1) & 0x55555555);
            input = (input & 0x33333333) + ((input >> 2) & 0x33333333);
            return (((input + (input >> 4) & 0xF0F0F0F) * 0x1010101) >> 24);
        }

        static u16 FloatToHalf(float a)
        {
            // https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
            u32 casted = *(u32*)&a;
            const u32 b = casted + 0x00001000;
            const u32 e = (b & 0x7F800000) >> 23;
            const u32 m = b & 0x007FFFFF;
            return (b & 0x80000000) >> 16 |
                   (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
                   ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
                   (e > 143) * 0x7FFF;
        }

        static u32 PackFloats(float a, float b)
        {
            return ((u32)FloatToHalf(b) << 16) | (u32)FloatToHalf(a);
        }
    };
}
