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
        using ManagedObject_DestroyClientObjectFn = void (*)(uptr);

        EntryPoint_LoadClientPluginFn EntryPoint_LoadClientPlugin;
        EntryPoint_UnloadClientPluginFn EntryPoint_UnloadClientPlugin;
        ManagedObject_InstantiateClientObjectFn ManagedObject_InstantiateClientObject;
        ManagedObject_DestroyClientObjectFn ManagedObject_DestroyClientObject;
    };
}