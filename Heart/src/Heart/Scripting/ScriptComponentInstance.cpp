#include "hepch.h"
#include "ScriptComponentInstance.h"

#include "Heart/Container/Variant.h"
#include "Heart/Container/HArray.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptComponentInstance::Instantiate()
    {
        if (!HasScriptClass()) return;
        Destroy();
        m_ObjectHandle = ScriptingEngine::InstantiateScriptComponent(
            GetScriptClassObject().GetFullName()
        );
    }

    const ScriptClass& ScriptComponentInstance::GetScriptClassObject() const
    {
        static ScriptClass defaultClass;
        auto found = ScriptingEngine::GetComponentClasses().find(m_ScriptClassId);
        if (found == ScriptingEngine::GetComponentClasses().end())
            return defaultClass;
        return found->second;
    }
}
