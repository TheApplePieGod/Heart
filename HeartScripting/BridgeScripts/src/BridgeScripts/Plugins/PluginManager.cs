using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.Loader;

namespace BridgeScripts.Plugins
{
    // https://docs.microsoft.com/en-us/dotnet/standard/assembly/unloadability
    public static class PluginManager
    {
        public static AssemblyLoadContext MainLoadContext { get; set; }
        public static List<string> SharedAssemblies { get; set; } = new List<string>();

        public static (bool, PluginLoadContext) LoadPlugin(string assemblyPath)
        {
            PluginLoadContext loadContext = new PluginLoadContext(assemblyPath);
            Assembly loaded = loadContext.LoadedAssembly;
            if (loaded == null)
                return (false, null);

            return (true, loadContext);
        }

        public static bool UnloadPlugin(ref PluginLoadContext loadContext)
        {
            if (loadContext == null)
                return true;

            // Keep a weak reference to the load context to check if gc has completed
            WeakReference alcWeakRef = loadContext.GetWeakReference();

            // Unload
            loadContext.Unload();
            loadContext = null;

            // Continually gc until everything is cleaned up
            int startTicks = Environment.TickCount;
            while (alcWeakRef.IsAlive)
            {
                GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced);
                GC.WaitForPendingFinalizers();

                if (!alcWeakRef.IsAlive) break;

                // Max out waiting at 4 seconds
                if (Environment.TickCount - startTicks >= 4000)
                    return false;
            }

            return true;
        }
    }
}
