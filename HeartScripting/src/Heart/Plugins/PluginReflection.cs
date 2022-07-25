using Heart.Core;
using Heart.Scene;
using System;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace Heart.Plugins
{
    public static class PluginReflection
    {
        public static void GetInstantiableClasses(Assembly assembly)
        {
            Type entityType = typeof(Entity);
            var types = assembly.GetTypes().Where(t => t.IsAssignableTo(entityType));
            foreach (var type in types)
            {
                Log.Warn(type.FullName);
            }
        }
    }
}
