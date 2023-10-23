#include "hepch.h"
#include "ScriptClass.h"

#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Container/HArray.h"
#include "Heart/Util/StringUtils.hpp"

namespace Heart
{
    ScriptClass::ScriptClass(const HString& fullName, s64 uniqueId)
        : m_FullName(fullName), m_UniqueId(uniqueId)
    {
        // Parse name from full name (last word after .)
        m_Name = fullName.GetViewUTF8().Split(".").Back().Data();

        ReloadSerializableFields();
    }

    void ScriptClass::ReloadSerializableFields()
    {
        m_SerializableFields.Clear();

        HArray outFields;
        ScriptingEngine::s_CoreCallbacks.PluginReflection_GetClientSerializableFields(&m_FullName, &outFields);
        for (u32 i = 0; i < outFields.Count(); i++)
            m_SerializableFields.Add(outFields[i].String().Convert(HString::Encoding::UTF8));
    }   
}
