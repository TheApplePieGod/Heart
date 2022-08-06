#include "hepch.h"
#include "ScriptInstance.h"

#include "Heart/Core/Timestep.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/Variant.h"
#include "Heart/Container/HArray.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptInstance::Instantiate(Entity owner)
    {
        if (!IsInstantiable()) return;
        if (IsAlive()) Destroy();
        m_ObjectHandle = ScriptingEngine::InstantiateObject(
            m_ScriptClass,
            (u32)owner.GetHandle(),
            owner.GetScene()
        );
    }

    void ScriptInstance::Destroy()
    {
        if (!IsAlive()) return;
        ScriptingEngine::DestroyObject(m_ObjectHandle);
        m_ObjectHandle = 0;
    }

    void ScriptInstance::Clear()
    {
        if (IsAlive()) Destroy();
        m_ScriptClass.Clear();
    }

    void ScriptInstance::OnPlayStart()
    {
        if (!IsAlive()) return;
        HArray args;
        ScriptingEngine::InvokeFunction(m_ObjectHandle, "OnPlayStart", args);
    }

    void ScriptInstance::OnPlayEnd()
    {
        if (!IsAlive()) return;
        HArray args;
        ScriptingEngine::InvokeFunction(m_ObjectHandle, "OnPlayEnd", args);
    }

    void ScriptInstance::OnUpdate(Timestep ts)
    {
        if (!IsAlive()) return;
        ScriptingEngine::InvokeEntityOnUpdate(m_ObjectHandle, ts);
    }
    
    nlohmann::json ScriptInstance::SerializeFieldsToJson()
    {
        nlohmann::json j;
        if (!IsAlive()) return j;

        auto& scriptClassObj = ScriptingEngine::GetInstantiableClass(m_ScriptClass);
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

        auto& scriptClassObj = ScriptingEngine::GetInstantiableClass(m_ScriptClass);
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
            SetFieldValueUnchecked(it.key(), it.value());
        }
    }

    void ScriptInstance::LoadFieldsFromBinary(void* data)
    {
        if (!data || !IsAlive()) return;

        HE_ENGINE_ASSERT(false, "Not implemented");
    }

    bool ScriptInstance::ValidateClass()
    {
        return Heart::ScriptingEngine::IsClassInstantiable(m_ScriptClass);
    }

    Variant ScriptInstance::GetFieldValue(const HStringView& fieldName) const
    {
        if (!IsAlive()) return Variant();
        return GetFieldValueUnchecked(fieldName);
    }

    bool ScriptInstance::SetFieldValue(const HStringView& fieldName, const Variant& value)
    {
        if (!IsAlive()) return false;
        return SetFieldValueUnchecked(fieldName, value);
    }
    
    Variant ScriptInstance::GetFieldValueUnchecked(const HStringView& fieldName) const
    {
        return ScriptingEngine::GetFieldValue(m_ObjectHandle, fieldName);
    }

    bool ScriptInstance::SetFieldValueUnchecked(const HStringView& fieldName, const Variant& value)
    {
        return ScriptingEngine::SetFieldValue(m_ObjectHandle, fieldName, value);
    }
}