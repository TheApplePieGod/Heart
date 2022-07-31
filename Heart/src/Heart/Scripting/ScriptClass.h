#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"

namespace Heart
{
    class ScriptClass
    {
    public:
        ScriptClass() = default;

        ScriptClass(const HString& fullName)
            : m_FullName(fullName)
        { ReloadSerializableFields(); }

        void ReloadSerializableFields();

    private:
        HString m_FullName;
        HVector<HString> m_SerializableFields;
    };
}