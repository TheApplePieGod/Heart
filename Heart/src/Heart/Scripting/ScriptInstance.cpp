#include "hepch.h"
#include "ScriptInstance.h"

#include "Heart/Core/Timestep.h"
#include "Heart/Container/Variant.h"
#include "Heart/Container/HArray.hpp"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptInstance::Instantiate()
    {
        if (!IsInstantiable()) return;
        if (IsAlive()) Destroy();
        m_ObjectHandle = ScriptingEngine::InstantiateObject(m_ScriptClass);
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
    
    Variant ScriptInstance::GetFieldValue(const HString& fieldName)
    {
        if (!IsAlive()) return Variant();
        return ScriptingEngine::GetFieldValue(m_ObjectHandle, fieldName);
    }

    bool ScriptInstance::SetFieldValue(const HString& fieldName, const Variant& value)
    {
        if (!IsAlive()) return false;
        return ScriptingEngine::SetFieldValue(m_ObjectHandle, fieldName, value);
    }
}