#pragma once

namespace Heart
{
    class HArray;
    class HString;
    class Variant;
    struct ManagedCallbacks
    {
        using EntryPoint_LoadClientPluginFn = bool (*)(const char8*, HArray*);
        using EntryPoint_UnloadClientPluginFn = bool (*)();
        using PluginReflection_GetClientSerializableFieldsFn = void (*)(const HString*, HArray*);
        using ManagedObject_InstantiateClientObjectFn = void* (*)(const HString*);
        using ManagedObject_DestroyObjectFn = void (*)(uptr);
        using ManagedObject_InvokeFunctionFn = bool (*)(uptr, const HString*, const HArray*);
        using ManagedObject_GetFieldValueFn = void (*)(uptr, const HString*, Variant*);
        using ManagedObject_SetFieldValueFn = bool (*)(uptr, const HString*, Variant);
        using Entity_CallOnUpdateFn = void (*)(uptr, double);

        EntryPoint_LoadClientPluginFn EntryPoint_LoadClientPlugin;
        EntryPoint_UnloadClientPluginFn EntryPoint_UnloadClientPlugin;
        PluginReflection_GetClientSerializableFieldsFn PluginReflection_GetClientSerializableFields;
        ManagedObject_InstantiateClientObjectFn ManagedObject_InstantiateClientObject;
        ManagedObject_DestroyObjectFn ManagedObject_DestroyObject;
        ManagedObject_InvokeFunctionFn ManagedObject_InvokeFunction;
        ManagedObject_GetFieldValueFn ManagedObject_GetFieldValue;
        ManagedObject_SetFieldValueFn ManagedObject_SetFieldValue;
        Entity_CallOnUpdateFn Entity_CallOnUpdate;
    };
}