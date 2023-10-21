#pragma once

namespace Heart
{
    class HArray;
    class HString;
    class Variant;
    class Scene;
    struct CoreManagedCallbacks
    {
        using PluginReflection_GetClientInstantiableClassesFn = void (*)(HArray*);
        using PluginReflection_GetClientSerializableFieldsFn = void (*)(const HString*, HArray*);
        using ManagedObject_InstantiateClientScriptEntityFn = uptr (*)(const HString*, u32, Scene*);
        using ManagedObject_InstantiateClientScriptComponentFn = uptr (*)(const HString*);
        using ManagedObject_DestroyObjectFn = void (*)(uptr);
        using ManagedObject_InvokeFunctionFn = bool (*)(uptr, const HString*, const HArray*);
        using ManagedObject_GetFieldValueFn = void (*)(uptr, const HString*, Variant*);
        using ManagedObject_SetFieldValueFn = bool (*)(uptr, const HString*, Variant, bool);
        using ScriptEntity_CallOnUpdateFn = void (*)(uptr, double);
        using ScriptEntity_CallOnCollisionStartedFn = void (*)(uptr, u32, Scene*);
        using ScriptEntity_CallOnCollisionEndedFn = void (*)(uptr, u32, Scene*);

        PluginReflection_GetClientInstantiableClassesFn PluginReflection_GetClientInstantiableClasses;
        PluginReflection_GetClientSerializableFieldsFn PluginReflection_GetClientSerializableFields;
        ManagedObject_InstantiateClientScriptEntityFn ManagedObject_InstantiateClientScriptEntity;
        ManagedObject_InstantiateClientScriptComponentFn ManagedObject_InstantiateClientScriptComponent;
        ManagedObject_DestroyObjectFn ManagedObject_DestroyObject;
        ManagedObject_InvokeFunctionFn ManagedObject_InvokeFunction;
        ManagedObject_GetFieldValueFn ManagedObject_GetFieldValue;
        ManagedObject_SetFieldValueFn ManagedObject_SetFieldValue;
        ScriptEntity_CallOnUpdateFn ScriptEntity_CallOnUpdate;
        ScriptEntity_CallOnCollisionStartedFn ScriptEntity_CallOnCollisionStarted;
        ScriptEntity_CallOnCollisionEndedFn ScriptEntity_CallOnCollisionEnded;
    };

    struct BridgeManagedCallbacks
    {
        using EntryPoint_LoadCorePluginFn = bool (*)(CoreManagedCallbacks*);
        using EntryPoint_UnloadCorePluginFn = bool (*)();
        using EntryPoint_LoadClientPluginFn = bool (*)(const char8*);
        using EntryPoint_UnloadClientPluginFn = bool (*)();

        EntryPoint_LoadCorePluginFn EntryPoint_LoadCorePlugin;
        EntryPoint_UnloadCorePluginFn EntryPoint_UnloadCorePlugin;
        EntryPoint_LoadClientPluginFn EntryPoint_LoadClientPlugin;
        EntryPoint_UnloadClientPluginFn EntryPoint_UnloadClientPlugin;
    };

}
