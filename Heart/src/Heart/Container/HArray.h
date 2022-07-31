#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/Variant.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    class HArray
    {
    public:
        ~HArray() = default;
        HArray() = default;

        HArray(const HArray& other)
            : m_Data(other.m_Data)
        {}

        HArray(u32 elemCount, bool fill = true)
            : m_Data(elemCount, fill)
        {}

        HArray(std::initializer_list<Variant> list)
            : m_Data(list)
        {}

        inline void Add(const Variant& value) { m_Data.Add(value); }
        inline void Reserve(u32 allocCount) { m_Data.Reserve(allocCount); }
        inline void Clear(bool shrink = false) { m_Data.Clear(shrink); }
        inline void Resize(u32 elemCount, bool construct = true) { m_Data.Resize(elemCount, construct); }
        inline HArray Clone() const { return HArray(m_Data.Clone()); }
        inline u32 GetCount() const { return m_Data.GetCount(); }
        inline Variant* Data() const { return m_Data.Data(); }
        inline Variant* Begin() const { return m_Data.Begin(); }
        inline Variant* End() const { return m_Data.End(); }
        inline Variant* Front() const { return m_Data.Begin(); }
        inline Variant* Back() const { return GetCount() > 0 ? m_Data.End() - 1 : m_Data.Begin(); }
        inline Variant& Get(u32 index) const { return m_Data.Get(index); }

        inline Variant& operator[](u32 index) const { return m_Data[index]; }
        inline void operator=(const HArray& other) { m_Data = other.m_Data; }

    private:
        HArray(const HVector<Variant>& data)
            : m_Data(data)
        {}

    private:
        HVector<Variant> m_Data;
    };

    void to_json(nlohmann::json& j, const HArray& str);
    void from_json(const nlohmann::json& j, HArray& str);
}