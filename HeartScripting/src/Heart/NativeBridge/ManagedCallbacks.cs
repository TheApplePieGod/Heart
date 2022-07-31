using Heart.Container;
using Heart.NativeInterop;
using Heart.Scene;
using System;

namespace Heart.NativeBridge
{
    internal unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<IntPtr, HArrayInternal*, InteropBool> EntryPoint_LoadClientPlugin;
        public delegate* unmanaged<InteropBool> EntryPoint_UnloadClientPlugin;
        public delegate* unmanaged<HStringInternal*, IntPtr> ManagedObject_InstantiateClientObject;
        public delegate* unmanaged<IntPtr, void> ManagedObject_DestroyObject;
        public delegate* unmanaged<IntPtr, HStringInternal*, HArrayInternal*, InteropBool> ManagedObject_InvokeFunction;
        public delegate* unmanaged<IntPtr, double, void> Entity_CallOnUpdate;

        public static ManagedCallbacks Get()
        {
            return new()
            {
                EntryPoint_LoadClientPlugin = &EntryPoint.LoadClientPlugin,
                EntryPoint_UnloadClientPlugin = &EntryPoint.UnloadClientPlugin,
                ManagedObject_InstantiateClientObject = &ManagedObject.InstantiateClientObject,
                ManagedObject_DestroyObject = &ManagedObject.DestroyObject,
                ManagedObject_InvokeFunction = &ManagedObject.InvokeFunction,
                Entity_CallOnUpdate = &Entity.CallOnUpdate
            };
        }
    }
}
