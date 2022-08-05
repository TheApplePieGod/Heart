#pragma once

#include "Heart/Scripting/ManagedCallbacks.h"
#include "Heart/Scripting/ScriptClass.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"

namespace Heart
{
    class Variant;
    class Timestep;
    class Scene;
    class ScriptingEngine
    {
    public:
        static void Initialize();
        static void Shutdown();

        static bool LoadClientPlugin(const HStringView& absolutePath);
        static bool UnloadClientPlugin();

        static uptr InstantiateObject(const HString& type, u32 entityHandle, Scene* sceneHandle);
        static void DestroyObject(uptr handle);
        static bool InvokeFunction(uptr object, const HString& funcName, const HArray& args);
        static void InvokeEntityOnUpdate(uptr entity, Timestep timestep);
        static Variant GetFieldValue(uptr entity, const HString& fieldName);
        static bool SetFieldValue(uptr entity, const HString& fieldName, const Variant& value);

        inline static bool IsClassInstantiable(const HStringView& name) { return s_InstantiableClasses.find(name) != s_InstantiableClasses.end(); }
        inline static ScriptClass& GetInstantiableClass(const HStringView& name) { return s_InstantiableClasses[name]; }
        inline static const auto& GetInstantiableClasses() { return s_InstantiableClasses; }
        inline static bool IsScriptInputEnabled() { return s_ScriptInputEnabled; }
        inline static void SetScriptInputEnabled(bool enabled) { s_ScriptInputEnabled = enabled; }

    private:
        inline static ManagedCallbacks s_CoreCallbacks;
        inline static bool s_ClientPluginLoaded;
        inline static std::unordered_map<HString, ScriptClass> s_InstantiableClasses;
        inline static bool s_ScriptInputEnabled = true;

        friend class ScriptClass;
    };
}