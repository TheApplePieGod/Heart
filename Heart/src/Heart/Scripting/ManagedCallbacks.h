#pragma once

namespace Heart
{
    struct ManagedCallbacks
    {
        using AssemblyManager_LoadAssemblyFn = bool (*)(char8*);

        AssemblyManager_LoadAssemblyFn AssemblyManager_LoadAssembly;
    };
}