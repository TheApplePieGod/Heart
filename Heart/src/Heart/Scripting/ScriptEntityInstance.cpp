#include "hepch.h"
#include "ScriptEntityInstance.h"

#include "Heart/Core/Timestep.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Container/Variant.h"
#include "Heart/Container/HArray.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptEntityInstance::Instantiate(Entity owner)
    {
        if (!HasScriptClass()) return;
        Destroy();
        m_ObjectHandle = ScriptingEngine::InstantiateScriptEntity(
            GetScriptClassObject().GetFullName(),
            (u32)owner.GetHandle(),
            owner.GetScene()
        );
    }

    void ScriptEntityInstance::OnConstruct()
    {
        if (!IsAlive()) return;
        HArray args;
        ScriptingEngine::InvokeFunction(m_ObjectHandle, "OnConstruct", args);
    }

    void ScriptEntityInstance::OnPlayStart()
    {
        if (!IsAlive()) return;
        HArray args;
        ScriptingEngine::InvokeFunction(m_ObjectHandle, "OnPlayStart", args);
    }

    void ScriptEntityInstance::OnPlayEnd()
    {
        if (!IsAlive()) return;
        HArray args;
        ScriptingEngine::InvokeFunction(m_ObjectHandle, "OnPlayEnd", args);
    }

    void ScriptEntityInstance::OnUpdate(Timestep ts)
    {
        if (!IsAlive()) return;
        ScriptingEngine::InvokeEntityOnUpdate(m_ObjectHandle, ts);
    }

    void ScriptEntityInstance::OnCollisionStarted(Entity other)
    {
        if (!IsAlive()) return;
        ScriptingEngine::InvokeEntityOnCollisionStarted(m_ObjectHandle, other);
    }

    void ScriptEntityInstance::OnCollisionEnded(Entity other)
    {
        if (!IsAlive()) return;
        ScriptingEngine::InvokeEntityOnCollisionEnded(m_ObjectHandle, other);
    }

    const ScriptClass& ScriptEntityInstance::GetScriptClassObject() const
    {
        static ScriptClass defaultClass;
        auto found = ScriptingEngine::GetEntityClasses().find(m_ScriptClassId);
        if (found == ScriptingEngine::GetEntityClasses().end())
            return defaultClass;
        return found->second;
    }
}
