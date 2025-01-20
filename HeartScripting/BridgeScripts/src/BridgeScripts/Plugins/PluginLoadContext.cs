using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.Loader;

namespace BridgeScripts.Plugins
{
    public class PluginLoadContext
    {
        // https://docs.microsoft.com/en-us/dotnet/core/tutorials/creating-app-with-plugin-support
        private class PluginLoadContextInternal : AssemblyLoadContext
        {
            private AssemblyDependencyResolver _resolver;

            public PluginLoadContextInternal(string pluginPath)
                : base(isCollectible: true)
            {
                _resolver = new AssemblyDependencyResolver(pluginPath);
            }

            public Assembly LoadWithoutLocking(string pluginPath)
            {
                // Load file directly to prevent file lock
                using FileStream assemblyStream = File.OpenRead(pluginPath);

                // Also load debug symbols if applicable
                string pdbPath = Path.ChangeExtension(pluginPath, "pdb");
                if (File.Exists(pdbPath))
                {
                    using FileStream pdbStream = File.OpenRead(pdbPath);
                    return LoadFromStream(assemblyStream, pdbStream);
                }

                return LoadFromStream(assemblyStream);
            }

            protected override Assembly Load(AssemblyName assemblyName)
            {
                if (assemblyName == null)
                    return null;

                if (PluginManager.SharedAssemblies.Contains(assemblyName.Name))
                    return PluginManager.MainLoadContext.LoadFromAssemblyName(assemblyName);

                string assemblyPath = _resolver.ResolveAssemblyToPath(assemblyName);
                if (assemblyPath == null)
                    return null;

                return LoadWithoutLocking(assemblyPath);
            }

            protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
            {
                // Try to use custom dll import resolver first
                // TODO: the passed assembly is probably not what it should be
                IntPtr loaded = EntryPoint.DllImportResolver.OnResolveDllImport(unmanagedDllName, Assembly.GetExecutingAssembly(), null);
                if (loaded != IntPtr.Zero)
                    return loaded;

                string libPath = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
                if (libPath != null)
                    return LoadUnmanagedDllFromPath(libPath);

                return IntPtr.Zero;
            }
        }

        private PluginLoadContextInternal _loadContext;
        private Assembly _loadedAssembly;

        public Assembly LoadedAssembly
        {
            [MethodImpl(MethodImplOptions.NoInlining)]
            get => _loadedAssembly;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public PluginLoadContext(string pluginPath)
        {
            _loadContext = new PluginLoadContextInternal(pluginPath);
            _loadedAssembly = _loadContext.LoadWithoutLocking(pluginPath);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void Unload()
        {
            if (_loadContext == null) return;

            _loadContext.Unload();
            _loadContext = null;
            _loadedAssembly = null;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public WeakReference GetWeakReference()
        {
            return new WeakReference(_loadContext, true);
        }
    }
}
