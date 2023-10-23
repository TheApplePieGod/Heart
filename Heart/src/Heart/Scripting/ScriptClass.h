#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"

namespace Heart
{
    class ScriptClass
    {
    public:
        ScriptClass() = default;

        ScriptClass(const HString& fullName, s64 uniqueId);

        void ReloadSerializableFields();

        inline s64 GetUniqueId() const { return m_UniqueId; }
        inline const HString& GetName() const { return m_Name; }
        inline const HString& GetFullName() const { return m_FullName; }
        inline const HVector<HString>& GetSerializableFields() const { return m_SerializableFields; }

    private:
        s64 m_UniqueId = 0;
        HString m_Name = "";
        HString m_FullName = "";
        HVector<HString> m_SerializableFields;
    };
}
