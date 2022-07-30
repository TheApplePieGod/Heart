#pragma once

namespace Heart
{
    class HArray;
    struct ManagedCallbacks
    {
        using EntryPoint_LoadClientPluginFn = bool (*)(const char8*, HArray*);
        using EntryPoint_UnloadClientPluginFn = bool (*)();
        using ManagedObject_InstantiateClientObjectFn = void* (*)(const char8*);

        EntryPoint_LoadClientPluginFn EntryPoint_LoadClientPlugin;
        EntryPoint_UnloadClientPluginFn EntryPoint_UnloadClientPlugin;
        ManagedObject_InstantiateClientObjectFn ManagedObject_InstantiateClientObject;
    };
}