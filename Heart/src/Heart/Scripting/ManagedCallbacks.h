#pragma once

namespace Heart
{
    class HArray;
    class HString;
    struct ManagedCallbacks
    {
        using EntryPoint_LoadClientPluginFn = bool (*)(const char8*, HArray*);
        using EntryPoint_UnloadClientPluginFn = bool (*)();
        using ManagedObject_InstantiateClientObjectFn = void* (*)(const HString*);
        using ManagedObject_DestroyObjectFn = void (*)(uptr);
        using ManagedObject_InvokeFunctionFn = bool (*)(uptr, const HString*, const HArray*);

        EntryPoint_LoadClientPluginFn EntryPoint_LoadClientPlugin;
        EntryPoint_UnloadClientPluginFn EntryPoint_UnloadClientPlugin;
        ManagedObject_InstantiateClientObjectFn ManagedObject_InstantiateClientObject;
        ManagedObject_DestroyObjectFn ManagedObject_DestroyObject;
        ManagedObject_InvokeFunctionFn ManagedObject_InvokeFunction;
    };
}