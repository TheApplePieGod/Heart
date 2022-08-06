#include "hepch.h"
#include "ScriptClass.h"

#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Container/HArray.h"

namespace Heart
{
    void ScriptClass::ReloadSerializableFields()
    {
        m_SerializableFields.Clear();

        HArray outFields;
        ScriptingEngine::s_CoreCallbacks.PluginReflection_GetClientSerializableFields(&m_FullName, &outFields);
        for (u32 i = 0; i < outFields.Count(); i++)
            m_SerializableFields.Add(outFields[i].String().Convert(HString::Encoding::UTF8));
    }   
}