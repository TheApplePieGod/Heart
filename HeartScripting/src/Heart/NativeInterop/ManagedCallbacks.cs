using System;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public unsafe struct ManagedCallbacks
    {
        public delegate* unmanaged<IntPtr, InteropBool> AssemblyManager_LoadAssembly;

        public static ManagedCallbacks Get()
        {
            return new()
            {
                AssemblyManager_LoadAssembly = &AssemblyManager.LoadAssembly
            };
        }
    }
}
