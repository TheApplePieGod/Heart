#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/Variant.h"

namespace Heart
{
    class HArray
    {
    public:
        ~HArray() = default;

        HArray()
        {
            // HArray will always have data allocated
            m_Data.Reserve(16);
        }

        HArray(const HArray& other)
        {
            m_Data = HVector<Variant>(other.m_Data);
        }

        inline void Add(const Variant& value) { m_Data.Add(value); }

    private:

    private:
        HVector<Variant> m_Data;
    };
}