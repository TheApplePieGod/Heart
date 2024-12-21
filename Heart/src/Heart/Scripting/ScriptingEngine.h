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
    class Entity;
    class ScriptingEngine
    {
    public:
        static bool Initialize();
        static void Shutdown();

        static bool LoadClientPlugin(const HStringView8& absolutePath);
        static bool UnloadClientPlugin();
        static bool ReloadCorePlugin();

        static uptr InstantiateScriptComponent(const HString& type);
        static uptr InstantiateScriptEntity(const HString& type, u32 entityHandle, Scene* sceneHandle);
        static void DestroyObject(uptr handle);
        static bool InvokeFunction(uptr object, const HString& funcName, const HArray& args);
        static void InvokeEntityOnUpdate(uptr entity, Timestep timestep);
        static void InvokeEntityOnCollisionStarted(uptr entity, Entity other);
        static void InvokeEntityOnCollisionEnded(uptr entity, Entity other);
        static Variant GetFieldValue(uptr entity, const HString& fieldName);
        static bool SetFieldValue(uptr entity, const HString& fieldName, const Variant& value, bool invokeCallback);

        static bool IsClassIdInstantiable(s64 classId);
        static s64 GetClassIdFromName(const HString& name);

        inline static const auto& GetEntityClasses() { return s_EntityClasses; }
        inline static const auto& GetComponentClasses() { return s_ComponentClasses; }

        inline static bool IsScriptInputEnabled() { return s_ScriptInputEnabled; }
        inline static void SetScriptInputEnabled(bool enabled) { s_ScriptInputEnabled = enabled; }

    private:
        static void FindDotnetInstallations();

    private:
        inline static BridgeManagedCallbacks s_BridgeCallbacks;
        inline static CoreManagedCallbacks s_CoreCallbacks;
        inline static bool s_ClientPluginLoaded = false;
        inline static std::unordered_map<HString, s64> s_NameToId;
        inline static std::unordered_map<s64, ScriptClass> s_EntityClasses;
        inline static std::unordered_map<s64, ScriptClass> s_ComponentClasses;
        inline static bool s_ScriptInputEnabled = true;
        inline static HString8 s_DotnetPath;

        friend class ScriptClass;
    };
}
