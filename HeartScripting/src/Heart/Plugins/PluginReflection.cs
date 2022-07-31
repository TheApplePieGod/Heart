using Heart.Container;
using Heart.Core;
using Heart.NativeInterop;
using Heart.Scene;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

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

        public static List<string> GetSerializableFields(Assembly assembly, string typeName)
        {
            Type serializeType = typeof(SerializeFieldAttribute);
            List<string> names = new();
            var fields = assembly.GetType(typeName)
                            .GetFields()
                            .Where(f => f.IsPublic || f.CustomAttributes
                                                        .Where(a => a.AttributeType == serializeType)
                                                        .Any());
            foreach (var field in fields)
            {
                names.Add(field.Name);
            }

            return names;
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetClientSerializableFields(HStringInternal* typeNameStr, HArrayInternal* outFields)
        {
            if (EntryPoint.ClientAssembly == null) return;

            string typeName = NativeMarshal.HStringInternalToString(*typeNameStr);
            var fields = PluginReflection.GetSerializableFields(EntryPoint.ClientAssembly, typeName);

            using var arr = new HArray(fields);
            arr.CopyTo(outFields);
        }
    }
}
