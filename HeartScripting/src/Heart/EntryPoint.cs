using Heart.Core;
using Heart.NativeInterop;
using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Heart
{
    public static class EntryPoint
    {
        [UnmanagedCallersOnly]
        private static unsafe InteropBool Initialize(IntPtr dllHandle, ManagedCallbacks* managedCallbacks)
        {
            Assembly coreAssembly = typeof(EntryPoint).Assembly;
            NativeLibrary.SetDllImportResolver(coreAssembly, new HeartDllImportResolver(dllHandle).OnResolveDllImport);

            *managedCallbacks = ManagedCallbacks.Get();

            return InteropBool.True;
        }
    }
}
