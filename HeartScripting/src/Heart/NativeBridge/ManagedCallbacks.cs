using Heart.Container;
using Heart.NativeInterop;
using System;

namespace Heart.NativeBridge
{
    internal unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<IntPtr, HArrayInternal*, InteropBool> EntryPoint_LoadClientPlugin;
        public delegate* unmanaged<InteropBool> EntryPoint_UnloadClientPlugin;
        public delegate* unmanaged<IntPtr, IntPtr> ManagedObject_InstantiateClientObject;

        public static ManagedCallbacks Get()
        {
            return new()
            {
                EntryPoint_LoadClientPlugin = &EntryPoint.LoadClientPlugin,
                EntryPoint_UnloadClientPlugin = &EntryPoint.UnloadClientPlugin,
                ManagedObject_InstantiateClientObject = &ManagedObject.InstantiateClientObject
            };
        }
    }
}
