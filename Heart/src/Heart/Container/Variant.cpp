#include "hepch.h"
#include "Variant.h"

#include "Heart/Container/HArray.h"

namespace Heart
{
    Variant::Variant(bool value)
    { m_Type = Type::Bool; m_Data.Bool = (byte)value; }
    Variant::Variant(int value)
    { m_Type = Type::Int; m_Data.Int = value; }
    Variant::Variant(float value)
    { m_Type = Type::Float; m_Data.Float = value; }
    Variant::Variant(const HArray& array)
    {
        m_Type = Type::Array;
        *reinterpret_cast<HArray*>(m_Data.Any) = array;
    }
    Variant::Variant(const Variant& other)
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
            case Type::Float:
            { m_Data.Float = other.Float(); } return;
            case Type::Array:
            { HE_PLACEMENT_NEW(m_Data.Any, HArray, other.Array()); } return;
        }

        HE_ENGINE_ASSERT("Variant copy constructor not fully implemented");
    }

    Variant::~Variant()
    {
        switch (m_Type)
        {
            case Type::None:
            case Type::Bool:
            case Type::Int:
            case Type::Float:
                return;
            case Type::Array:
            { reinterpret_cast<HArray*>(m_Data.Any)->~HArray(); }
        }

         return;
        HE_ENGINE_ASSERT("Variant destructor not fully implemented");
    }

    HArray Variant::Array() const
    { return *reinterpret_cast<const HArray*>(m_Data.Any); }
}