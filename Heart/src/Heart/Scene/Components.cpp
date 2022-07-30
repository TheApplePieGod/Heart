#include "hepch.h"
#include "Components.h"

#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptComponent::InstantiateObject()
    {
        if (ObjectHandle != 0) DestroyObject();
        ObjectHandle = ScriptingEngine::InstantiateObject(ObjectType);
    }

    void ScriptComponent::DestroyObject()
    {
        if (ObjectHandle == 0) return;
        ScriptingEngine::DestroyObject(ObjectHandle);
        ObjectHandle = 0;
    }
}