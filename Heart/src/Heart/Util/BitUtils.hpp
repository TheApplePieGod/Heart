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
    };
}
