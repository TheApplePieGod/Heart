#pragma once

namespace Heart
{
    enum class StringComparison : byte
    {
        Value = 0,
        Alphabetical,
        Equality
    };

    struct StringUtils
    {
        inline static constexpr u32 InvalidIndex = std::numeric_limits<u32>::max();

        template <typename T>
        static constexpr u32 Find(const T* str1, u32 len1, const T* str2, u32 len2)
        {
            if (!str1 || !str2) return InvalidIndex;
            if (len1 == 0 || len2 == 0) return InvalidIndex;
            u32 strPtr = 0;
            u32 searchPtr = 0;
            while (strPtr < len1)
            {
                if (str1[strPtr] == str2[searchPtr])
                    searchPtr++;
                else
                    searchPtr = 0;
                strPtr++;

                if (searchPtr == len2)
                    return strPtr - len2;
            }
            return InvalidIndex;
        }

        template <typename T>
        static constexpr int ToInt(const T* str, u32 len)
        {
            if (!str) return 0;
            if (len == 0) return 0;
            int value = 0;
            for (u32 ptr = len - 1; ptr > 0; ptr--)
                value += GetNumericValue(str[ptr]) * pow(10, len - ptr - 1);
            if (str[0] == '-')
                value *= -1;
            else
                value += GetNumericValue(str[0]) * pow(10, len - 1);
            return value;
        }

        template <typename T>
        static constexpr u32 StrLen(const T* str)
        {
            if (!str) return 0;
            u32 len = 0;
            while (str[len] != (T)'\0') len++;
            return len;
        }

        template <typename T>
        static constexpr bool CompareEq(const T* str1, u32 len1, const T* str2, u32 len2)
        {
            if (!str1 || !str2) return false;
            if (len1 != len2) return false;
            u32 ptr = 0;
            while (ptr < len1 && ptr < len2)
            {
                if (str1[ptr] != str2[ptr]) return false;
                ptr++;
            }
            return true;
        }

        // 0 if eq, -1 if str1 less, +1 if str1 greater
        template <typename T>
        static constexpr int CompareByValue(const T* str1, u32 len1, const T* str2, u32 len2)
        {
            if (!str1 || !str2) return 1;
            u32 ptr = 0;
            while (ptr < len1 && ptr < len2)
            {
                if (str1[ptr] < str2[ptr]) return -1;
                if (str1[ptr] > str2[ptr]) return 1;
                ptr++;
            }
            return 0;
        }

        // 0 if eq, -1 if str1 less, +1 if str1 greater
        // only supported for standard english
        template <typename T>
        static constexpr int CompareAlphabetical(const T* str1, u32 len1, const T* str2, u32 len2)
        {
            if (!str1 || !str2) return 1;
            u32 ptr = 0;
            while (ptr < len1 && ptr < len2)
            {
                // Ensure digits are sorted in order
                if (IsDigit(str1[ptr]) && IsDigit(str2[ptr]))
                {
                    u32 ptr1 = ptr;
                    u32 ptr2 = ptr;
                    while (true)
                    {
                        bool loop = false;
                        if (ptr1 < len1 && IsDigit(str1[ptr1++]))
                            loop = true;
                        if (ptr2 < len2 && IsDigit(str2[ptr2++]))
                            loop = true;
                        if (!loop) break;
                    }

                    int num1 = ToInt(str1 + ptr, ptr1 - ptr);
                    int num2 = ToInt(str2 + ptr, ptr2 - ptr);
                    if (num1 < num2) return -1;
                    if (num1 > num2) return 1;

                    // If the nums are equal, we can move the comparison pointer
                    // to after the end of the number
                    ptr = ptr1;
                }
                else
                {
                    // Convert to lowercase for comparison if applicable
                    T str1Val = str1[ptr];
                    if (IsAsciiUppercase(str1Val))
                        str1Val += 32;
                    T str2Val = str2[ptr];
                    if (IsAsciiUppercase(str2Val))
                        str2Val += 32;

                    if (str1Val < str2Val) return -1;
                    if (str1Val > str2Val) return 1;

                    ptr++;
                }
            }
            return 0;
        }

        template <typename T>
        inline static constexpr bool IsDigit(T value)
        {
            return value >= 48 && value <= 57;
        }

        template <typename T>
        inline static constexpr int GetNumericValue(T value)
        {
            return value - 48;
        }

        template <typename T>
        inline static constexpr bool IsAsciiUppercase(T value)
        {
            return value >= 65 && value <= 90;
        }
    };
}