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
        internal static unsafe InteropBool Initialize(IntPtr dllHandle, ManagedCallbacks* managedCallbacks)
        {
            _coreAssembly = Assembly.GetExecutingAssembly();
            NativeLibrary.SetDllImportResolver(_coreAssembly, new HeartDllImportResolver(dllHandle).OnResolveDllImport);

            PluginManager.MainLoadContext = AssemblyLoadContext.GetLoadContext(_coreAssembly);
            PluginManager.SharedAssemblies.Add(_coreAssembly.GetName().Name);

            *managedCallbacks = ManagedCallbacks.Get();

            // Test();
            // Test2();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced);
            GC.WaitForPendingFinalizers();

            return InteropBool.True;
        }
        
        // TODO: use HString
        [UnmanagedCallersOnly]
        internal static unsafe InteropBool LoadClientPlugin(IntPtr assemblyPathStr, HArrayInternal* outClasses)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathStr);

            (bool success, _clientLoadContext) = PluginManager.LoadPlugin(assemblyPath);
            if (success)
            {
                var instantiableClasses = PluginReflection.GetInstantiableClasses(_clientLoadContext.LoadedAssembly);
                using var arr = new HArray(instantiableClasses);
                arr.CopyTo(outClasses);
            }

            return NativeMarshal.BoolToInteropBool(success);
        }

        [UnmanagedCallersOnly]
        internal static InteropBool UnloadClientPlugin()
        {
            bool success = PluginManager.UnloadPlugin(ref _clientLoadContext);

            return NativeMarshal.BoolToInteropBool(success);
        }

        private static unsafe void Test()
        {
            HArray arr = new HArray();
            for (int i = 0; i < 10; i++)
            {
                using HArray arr2 = new HArray();
                arr2.Add(true);
                arr2.Add(8390213);
                arr2.Add("brejiment");
                arr.Add(arr2);
            }

            object value = arr[5];
            arr[5] = true;
            object value2 = arr[5];
        }

        private static unsafe void Test2()
        {
            using (HArray arr = new HArray())
            {
                int startTime = Environment.TickCount;

                for (int i = 0; i < 1000000; i++)
                {
                    arr.Add("brejiment");
                }

                int elapsed = Environment.TickCount - startTime;
                Log.Warn("Add strings took {0}ms", elapsed);
            }
            using (HArray arr = new HArray())
            {
                int startTime = Environment.TickCount;

                for (int i = 0; i < 1000000; i++)
                {
                    arr.Add(12345);
                }

                int elapsed = Environment.TickCount - startTime;
                Log.Warn("Add ints took {0}ms", elapsed);
            }
            using (HArray arr = new HArray())
            {
                int startTime = Environment.TickCount;

                for (int i = 0; i < 1000000; i++)
                {
                    arr.Add(new HArray());
                }

                int elapsed = Environment.TickCount - startTime;
                Log.Warn("Add nested arrays took {0}ms", elapsed);
            }
        }
    }
}
