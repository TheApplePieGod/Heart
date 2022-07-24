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
        private static int Initialize(IntPtr dllHandle)
        {
            Assembly coreAssembly = typeof(EntryPoint).Assembly;
            NativeLibrary.SetDllImportResolver(coreAssembly, new HeartDllImportResolver(dllHandle).OnResolveDllImport);

            Log.Info("C# scripting!");

            return 10;
        }
    }
}
