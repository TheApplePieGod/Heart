#include "hepch.h"
#include "Components.h"

#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void ScriptComponent::InstantiateObject()
    {
        if (ObjectHandle != 0) FreeObject();
        //ScriptingEngine::InstantiateClass(NamespaceName, ClassName, &ObjectHandle);
    }

    void ScriptComponent::FreeObject()
    {
        if (ObjectHandle == 0) return;
        //ScriptingEngine::FreeObjectHandle(ObjectHandle);
        ObjectHandle = 0;
    }
}