using Heart.Container;
using Heart.Core;
using Heart.NativeInterop;
using Heart.Scene;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Heart.NativeBridge
{
    internal static class ClientReflection
    {
        // Complains about no assignment but we assign it cross-assembly
#pragma warning disable CS0649
        internal static Assembly _clientAssembly;
#pragma warning restore CS0649

        internal static List<string> GetInstantiableClasses()
        {
            if (_clientAssembly == null) return null;

            Type scriptEntityType = typeof(ScriptEntity);
            List<string> names = new();
            var types = _clientAssembly.GetTypes().Where(t => t.IsAssignableTo(scriptEntityType));

            foreach (var type in types)
                names.Add(type.FullName);

            return names;
        }

        internal static List<string> GetSerializableFields(string typeName)
        {
            if (_clientAssembly == null) return null;

            Type serializeType = typeof(SerializeFieldAttribute);
            List<string> names = new();
            
            var type = _clientAssembly.GetType(typeName);
            if (type == null) return names;

            var fields = type.GetFields()
                            .Where(f => f.IsPublic || f.CustomAttributes
                                                        .Where(a => a.AttributeType == serializeType)
                                                        .Any());

            foreach (var field in fields)
                names.Add(field.Name);

            return names;
        }

        internal static Type GetClientType(string name)
        {
            if (_clientAssembly == null) return null;
            return _clientAssembly.GetType(name);
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetClientInstantiableClasses(HArrayInternal* outClasses)
        {
            if (_clientAssembly == null) return;

            var instantiableClasses = GetInstantiableClasses();
            using var arr = new HArray(instantiableClasses);
            arr.CopyTo(outClasses);
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetClientSerializableFields(HStringInternal* typeNameStr, HArrayInternal* outFields)
        {
            if (_clientAssembly == null) return;

            string typeName = NativeMarshal.HStringInternalToString(*typeNameStr);
            var fields = GetSerializableFields(typeName);

            using var arr = new HArray(fields);
            arr.CopyTo(outFields);
        }
    }
}
