#include "hepch.h"
#include "ScriptInstance.h"

#include "Heart/Container/Variant.h"
#include "Heart/Container/HArray.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptInstance::ConsumeHandle(uptr newHandle)
    {
        Destroy();
        m_ObjectHandle = newHandle;
    }

    void ScriptInstance::Destroy()
    {
        if (!IsAlive()) return;
        ScriptingEngine::DestroyObject(m_ObjectHandle);
        m_ObjectHandle = 0;
    }

    void ScriptInstance::Clear()
    {
        Destroy();
        m_ScriptClassId = 0;
    }

    nlohmann::json ScriptInstance::SerializeFieldsToJson()
    {
        nlohmann::json j;
        if (!IsAlive()) return j;

        auto& scriptClassObj = GetScriptClassObject();
        auto& fields = scriptClassObj.GetSerializableFields();

        for (u32 i = 0; i < fields.Count(); i++)
        {
            Variant value = GetFieldValueUnchecked(fields[i]);
            if (value.GetType() == Variant::Type::None) continue;
            j[fields[i].DataUTF8()] = value;
        }

        return j;
    }

    void* ScriptInstance::SerializeFieldsToBinary()
    {
        if (!IsAlive()) return nullptr;

        auto& scriptClassObj = GetScriptClassObject();
        auto& fields = scriptClassObj.GetSerializableFields();

        HE_ENGINE_ASSERT(false, "Not implemented");
    
        return nullptr;
    }

    void ScriptInstance::LoadFieldsFromJson(const nlohmann::json& j)
    {
        if (!IsAlive()) return;
        
        for (auto it = j.begin(); it != j.end(); it++)
        {
            Variant d = it.value();
            SetFieldValueUnchecked(it.key(), it.value(), false);
        }
    }

    void ScriptInstance::LoadFieldsFromBinary(void* data)
    {
        if (!data || !IsAlive()) return;

        HE_ENGINE_ASSERT(false, "Not implemented");
    }

    bool ScriptInstance::IsInstantiable()
    {
        if (!HasScriptClass()) return false;
        return Heart::ScriptingEngine::IsClassIdInstantiable(m_ScriptClassId);
    }

    Variant ScriptInstance::GetFieldValue(const HString& fieldName) const
    {
        if (!IsAlive()) return Variant();
        return GetFieldValueUnchecked(fieldName);
    }

    bool ScriptInstance::SetFieldValue(const HString& fieldName, const Variant& value, bool invokeCallback)
    {
        if (!IsAlive()) return false;
        return SetFieldValueUnchecked(fieldName, value, invokeCallback);
    }
    
    Variant ScriptInstance::GetFieldValueUnchecked(const HString& fieldName) const
    {
        return ScriptingEngine::GetFieldValue(m_ObjectHandle, fieldName);
    }

    bool ScriptInstance::SetFieldValueUnchecked(const HString& fieldName, const Variant& value, bool invokeCallback)
    {
        return ScriptingEngine::SetFieldValue(m_ObjectHandle, fieldName, value, invokeCallback);
    }
}
