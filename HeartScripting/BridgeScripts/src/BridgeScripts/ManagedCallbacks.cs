using System;

namespace BridgeScripts
{
    internal unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<IntPtr, byte> EntryPoint_LoadCorePlugin;
        public delegate* unmanaged<byte> EntryPoint_UnloadCorePlugin;
        public delegate* unmanaged<IntPtr, byte> EntryPoint_LoadClientPlugin;
        public delegate* unmanaged<byte> EntryPoint_UnloadClientPlugin;

        public static ManagedCallbacks Get()
        {
            return new()
            {
                EntryPoint_LoadCorePlugin = &EntryPoint.LoadCorePlugin,
                EntryPoint_UnloadCorePlugin = &EntryPoint.UnloadCorePlugin,
                EntryPoint_LoadClientPlugin = &EntryPoint.LoadClientPlugin,
                EntryPoint_UnloadClientPlugin = &EntryPoint.UnloadClientPlugin
            };
        }
    }
}
