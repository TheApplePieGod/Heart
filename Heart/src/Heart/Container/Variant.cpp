#include "hepch.h"
#include "Variant.h"

#include "Heart/Container/HArray.h"
#include "Heart/Container/HString.h"

namespace Heart
{
    Variant::Variant(bool value)
    { m_Type = Type::Bool; m_Data.Bool = (byte)value; }
    Variant::Variant(s8 value)
    { m_Type = Type::Int; m_Data.Int = (s64)value; }
    Variant::Variant(s16 value)
    { m_Type = Type::Int; m_Data.Int = (s64)value; }
    Variant::Variant(s32 value)
    { m_Type = Type::Int; m_Data.Int = (s64)value; }
    Variant::Variant(s64 value)
    { m_Type = Type::Int; m_Data.Int = value; }
    Variant::Variant(u8 value)
    { m_Type = Type::UInt; m_Data.UInt = (u64)value; }
    Variant::Variant(u16 value)
    { m_Type = Type::UInt; m_Data.UInt = (u64)value; }
    Variant::Variant(u32 value)
    { m_Type = Type::UInt; m_Data.UInt = (u64)value; }
    Variant::Variant(u64 value)
    { m_Type = Type::UInt; m_Data.UInt = value; }
    Variant::Variant(float value)
    { m_Type = Type::Float; m_Data.Float = (double)value; }
    Variant::Variant(double value)
    { m_Type = Type::Float; m_Data.Float = value; }
    Variant::Variant(const HArray& array)
    { m_Type = Type::Array; HE_PLACEMENT_NEW(m_Data.Any, HArray, array); }
    Variant::Variant(const HStringView& str)
    { m_Type = Type::String; HE_PLACEMENT_NEW(m_Data.Any, HString, str); }
    Variant::Variant(const HString& str)
    { m_Type = Type::String; HE_PLACEMENT_NEW(m_Data.Any, HString, str); }

    Variant::~Variant()
    {
        switch (m_Type)
        {
            case Type::None:
            case Type::Bool:
            case Type::Int:
            case Type::UInt:
            case Type::Float:
                return;
            case Type::Array:
            { reinterpret_cast<HArray*>(m_Data.Any)->~HArray(); } return;
            case Type::String:
            { reinterpret_cast<HString*>(m_Data.Any)->~HString(); } return;
        }

        return;
        HE_ENGINE_ASSERT(false, "Variant destructor not fully implemented");
    }

    HArray Variant::Array() const
    { return *reinterpret_cast<const HArray*>(m_Data.Any); }
    HString Variant::String() const
    { return *reinterpret_cast<const HString*>(m_Data.Any); }

    void Variant::Copy(const Variant& other)
    {
        m_Type = other.m_Type;
        switch (m_Type)
        {
            case Type::None:
                return;
            case Type::Bool:
            { m_Data.Bool = other.Bool(); } return;
            case Type::Int:
            { m_Data.Int = other.Int(); } return;
            case Type::UInt:
            { m_Data.UInt = other.UInt(); } return;
            case Type::Float:
            { m_Data.Float = other.Float(); } return;
            case Type::Array:
            { HE_PLACEMENT_NEW(m_Data.Any, HArray, other.Array()); } return;
            case Type::String:
            { HE_PLACEMENT_NEW(m_Data.Any, HString, other.String()); } return;
        }

        HE_ENGINE_ASSERT(false, "Variant copy constructor not fully implemented");
    }

    void to_json(nlohmann::json& j, const Variant& variant)
    {
        switch (variant.GetType())
        {
            default: return;
            case Variant::Type::Bool:
            { j = variant.Bool(); } return;
            case Variant::Type::Int:
            { j = variant.Int(); } return;
            case Variant::Type::UInt:
            { j = variant.UInt(); } return;
            case Variant::Type::Float:
            { j = variant.Float(); } return;
            case Variant::Type::Array:
            { j = variant.Array(); } return;
            case Variant::Type::String:
            { j = variant.String(); } return;
        }
    }

    void from_json(const nlohmann::json& j, Variant& variant)
    {
        switch (j.type())
        {
            default: return;
            case nlohmann::json::value_t::boolean:
            { variant = Variant(j.get<bool>()); } return;
            case nlohmann::json::value_t::number_integer:
            { variant = Variant(j.get<s64>()); } return;
            case nlohmann::json::value_t::number_unsigned:
            { variant = Variant(j.get<u64>()); } return;
            case nlohmann::json::value_t::number_float:
            { variant = Variant(j.get<double>()); } return;
            case nlohmann::json::value_t::array:
            { variant = Variant(j.get<HArray>()); } return;
            case nlohmann::json::value_t::string:
            { variant = Variant(j.get<HString>()); } return;
        }
    }
}