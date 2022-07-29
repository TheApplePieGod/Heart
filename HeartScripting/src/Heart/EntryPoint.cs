using Heart.Container;
using Heart.Core;
using Heart.NativeBridge;
using Heart.NativeInterop;
using Heart.Plugins;
using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Heart
{
    public static class EntryPoint
    {
        private static Assembly _coreAssembly;
        private static PluginLoadContext _clientLoadContext;

        public static Assembly CoreAssembly
        {
            get => _coreAssembly;
        }

        public static Assembly ClientAssembly
        {
            get => _clientLoadContext.LoadedAssembly;
        }


        [UnmanagedCallersOnly]
        private static unsafe InteropBool Initialize([In] IntPtr dllHandle, [In] ManagedCallbacks* managedCallbacks)
        {
            _coreAssembly = Assembly.GetExecutingAssembly();
            NativeLibrary.SetDllImportResolver(_coreAssembly, new HeartDllImportResolver(dllHandle).OnResolveDllImport);

            PluginManager.MainLoadContext = AssemblyLoadContext.GetLoadContext(_coreAssembly);
            PluginManager.SharedAssemblies.Add(_coreAssembly.GetName().Name);

            *managedCallbacks = ManagedCallbacks.Get();

            Test();

            return InteropBool.True;
        }

        [UnmanagedCallersOnly]
        public static InteropBool LoadClientPlugin([In] IntPtr assemblyPathStr)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathStr);

            (bool success, _clientLoadContext) = PluginManager.LoadPlugin(assemblyPath);

            return NativeMarshal.BoolToInteropBool(success);
        }

        [UnmanagedCallersOnly]
        public static InteropBool UnloadClientPlugin()
        {
            bool success = PluginManager.UnloadPlugin(ref _clientLoadContext);

            return NativeMarshal.BoolToInteropBool(success);
        }

        private static unsafe void Test()
        {
            int startTime = Environment.TickCount;
            HArray arr = new HArray();
            for (int i = 0; i < 1000000; i++)
            {
                HArray arr2 = new HArray();
                arr2.Add(true);
                arr2.Add(8390213);
                arr.Add(arr2);
            }
            int elapsed = Environment.TickCount - startTime;
            Log.Warn("Add took {0}ms", elapsed);
            arr.Destroy();
        }
    }
}
