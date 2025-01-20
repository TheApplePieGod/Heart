using BridgeScripts.Plugins;
using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Linq;
using System.Runtime.Serialization;
using System.IO;

namespace BridgeScripts
{
    public static class EntryPoint
    {
        private static PluginLoadContext _clientLoadContext;
        private static PluginLoadContext _coreLoadContext;
        private static DllImportResolver _dllImportResolver;

        public static DllImportResolver DllImportResolver
        {
            get => _dllImportResolver;
        }

        public static Assembly CoreAssembly
        {
            get => _coreLoadContext.LoadedAssembly;
        }

        public static Assembly ClientAssembly
        {
            get => _clientLoadContext.LoadedAssembly;
        }

        private static void UpdateCoreClientReference(object value)
        {
            _coreLoadContext.LoadedAssembly
                .GetType("Heart.NativeBridge.ClientReflection")
                .GetField("_clientAssembly", BindingFlags.Static | BindingFlags.NonPublic)
                .SetValue(null, value);
        }

        [UnmanagedCallersOnly]
        internal static unsafe byte Initialize(IntPtr dllHandle, ManagedCallbacks* managedCallbacks)
        {
            _dllImportResolver = new DllImportResolver(dllHandle);

            // Should be registered as shared even though it is not loaded by default
            PluginManager.SharedAssemblies.Add("CoreScripts");

            *managedCallbacks = ManagedCallbacks.Get();

            return 1;
        }

        [UnmanagedCallersOnly]
        internal static unsafe byte LoadCorePlugin(IntPtr managedCallbacks)
        {
            var coreScriptsPath = Path.Combine(Directory.GetParent(Assembly.GetExecutingAssembly().Location).FullName, "CoreScripts.dll");
            (bool success, _coreLoadContext) = PluginManager.LoadPlugin(coreScriptsPath);
            if (success)
            {
                // Update the main load context to be the core because all client assemblies should reference the same core plugin
                PluginManager.MainLoadContext = AssemblyLoadContext.GetLoadContext(_coreLoadContext.LoadedAssembly);

                // Update dll import resolver for future use
                NativeLibrary.SetDllImportResolver(_coreLoadContext.LoadedAssembly, _dllImportResolver.OnResolveDllImport);

                // Update the opaque managed callbacks handle
                _coreLoadContext.LoadedAssembly
                    .GetType("Heart.NativeBridge.ManagedCallbacks")
                    .GetMethod("Get")
                    .Invoke(null, new object[]{ managedCallbacks });
            }

            return (byte)(success ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte UnloadCorePlugin()
        {
            PluginManager.MainLoadContext = null;

            bool success = PluginManager.UnloadPlugin(ref _coreLoadContext);
            _coreLoadContext = null;

            return (byte)(success ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static unsafe byte LoadClientPlugin(IntPtr assemblyPathStr)
        {
            if (_coreLoadContext == null) return 0;

            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathStr);

            (bool success, _clientLoadContext) = PluginManager.LoadPlugin(assemblyPath);
            if (success)
            {
                NativeLibrary.SetDllImportResolver(_clientLoadContext.LoadedAssembly, _dllImportResolver.OnResolveDllImport);
                UpdateCoreClientReference(_clientLoadContext.LoadedAssembly);
            }

            return (byte)(success ? 1 : 0);
        }

        [UnmanagedCallersOnly]
        internal static byte UnloadClientPlugin()
        {
            if (_clientLoadContext == null) return 1;

            UpdateCoreClientReference(null);

            bool success = PluginManager.UnloadPlugin(ref _clientLoadContext);
            _clientLoadContext = null;

            return (byte)(success ? 1 : 0);
        }
    }
}
