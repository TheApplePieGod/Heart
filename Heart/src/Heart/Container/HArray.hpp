#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/Variant.h"

namespace Heart
{
    class HArray
    {
    public:
        ~HArray() = default;
        HArray() = default;

        HArray(u32 elemCount, bool fill = true)
            : m_Data(elemCount, fill)
        {}

        HArray(const HArray& other)
            : m_Data(other.m_Data)
        {}

        inline void Add(const Variant& value) { m_Data.Add(value); }

        inline Variant& operator[](u32 index) { return m_Data[index]; }
        inline void operator=(const HArray& other) { m_Data = other.m_Data; }

    private:

    private:
        HVector<Variant> m_Data;
    };
}