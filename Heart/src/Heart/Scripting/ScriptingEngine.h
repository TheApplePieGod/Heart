#pragma once

#include "Heart/Scripting/ManagedCallbacks.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"
#include "Heart/Core/Timestep.h"

namespace Heart
{
    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

        static bool LoadClientPlugin(const std::string& absolutePath);
        static bool UnloadClientPlugin();

        static uptr InstantiateObject(const HString& type);
        static void DestroyObject(uptr handle);
        static bool InvokeFunction(uptr object, const HString& funcName, const HArray& args);
        static void InvokeEntityOnUpdate(uptr entity, Timestep timestep);

        inline static const HVector<HString>& GetInstantiableClasses() { return s_InstantiableClasses; }

    private:
        inline static ManagedCallbacks s_CoreCallbacks;
        inline static bool s_ClientPluginLoaded;
        inline static HVector<HString> s_InstantiableClasses;
    };
}