using Heart.Core;
using Heart.Scene;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Heart.Plugins
{
    public static class PluginReflection
    {
        public static List<string> GetInstantiableClasses(Assembly assembly)
        {
            Type entityType = typeof(Entity);
            List<string> names = new();
            var types = assembly.GetTypes().Where(t => t.IsAssignableTo(entityType));
            foreach (var type in types)
            {
                names.Add(type.FullName);
            }
            return names;
        }
    }
}
