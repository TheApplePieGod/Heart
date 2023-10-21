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

        internal static List<(string, Int64)> GetScriptEntityClasses()
        {
            if (_clientAssembly == null) return null;

            Type scriptEntityType = typeof(ScriptEntity);
            var names = _clientAssembly.GetTypes()
                .Where(t => t.IsAssignableTo(scriptEntityType))
                .Select(t => (t.FullName, (Int64)t.GetField("GENERATED_UniqueId").GetValue(null)))
                .ToList();

            return names;
        }

        internal static List<(string, Int64)> GetScriptComponentClasses()
        {
            if (_clientAssembly == null) return null;

            var names = _clientAssembly.GetTypes()
                .Where(
                    t => t.IsClass && t.GetInterfaces().Any(i => i.FullName.StartsWith("Heart.Scene.IComponent"))
                )
                .Select(t => (t.FullName, (Int64)t.GetField("GENERATED_UniqueId").GetValue(null)))
                .ToList();

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
                    .Any() && !f.Name.StartsWith("GENERATED_")
                );

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
        internal static unsafe void GetClientInstantiableClasses(HArrayInternal* outArgs)
        {
            if (_clientAssembly == null) return;

            using var outArr = new HArray();
            using var ecNameArr = new HArray();
            using var ecIdArr = new HArray();
            using var scNameArr = new HArray();
            using var scIdArr = new HArray();

            foreach (var res in GetScriptEntityClasses())
            {
                ecNameArr.Add(res.Item1);
                ecIdArr.Add(res.Item2);
            }

            foreach (var res in GetScriptComponentClasses())
            {
                scNameArr.Add(res.Item1);
                scIdArr.Add(res.Item2);
            }

            outArr.Add(ecNameArr);
            outArr.Add(ecIdArr);
            outArr.Add(scNameArr);
            outArr.Add(scIdArr);

            outArr.CopyTo(outArgs);
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
