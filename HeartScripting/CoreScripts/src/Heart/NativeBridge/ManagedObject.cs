using Heart.Container;
using Heart.NativeInterop;
using Heart.Scene;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace Heart.NativeBridge
{
    // TODO: reflection caching
    public static class ManagedObject
    {
        [UnmanagedCallersOnly]
        internal static unsafe IntPtr InstantiateClientObject(HStringInternal* objectTypeStr)
        {
            if (EntryPoint.ClientAssembly == null) return IntPtr.Zero;

            string typeStr = NativeMarshal.HStringInternalToString(*objectTypeStr);
            Type objectType = EntryPoint.ClientAssembly.GetType(typeStr);
            if (objectType == null) return IntPtr.Zero;

            // Instantiate uninitialized object
            var instance = FormatterServices.GetUninitializedObject(objectType);

            // Find default parameterless constructor
            var constructor = objectType
                .GetConstructors(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                .Where(c => c.GetParameters().Length == 0)
                .FirstOrDefault();

            if (constructor != null)
                constructor.Invoke(instance, null);

            var handle = ManagedGCHandle.AllocStrong(instance);
            return handle.ToIntPtr();
        }

        [UnmanagedCallersOnly]
        internal static void DestroyObject(IntPtr objectHandle)
        {
            if (objectHandle == IntPtr.Zero) return;

            ManagedGCHandle.FromIntPtr(objectHandle).Free();
        }

        internal static unsafe MethodInfo FindFunction(ManagedGCHandle objectHandle, string funcName, int argCount)
        {
            var func = objectHandle.Target.GetType()
                .GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                .Where(m => m.Name == funcName && m.GetParameters().Length == argCount)
                .FirstOrDefault();

            return func;
        }

        [UnmanagedCallersOnly]
        internal static unsafe InteropBool InvokeFunction(IntPtr objectHandle, HStringInternal* funcNameStr, HArrayInternal* args)
        {
            var gcHandle = ManagedGCHandle.FromIntPtr(objectHandle);
            if (gcHandle != null && !gcHandle.IsAlive) return InteropBool.False;

            HArray argsArray = new HArray(*args);
            string funcName = NativeMarshal.HStringInternalToString(*funcNameStr);
            var func = FindFunction(gcHandle, funcName, argsArray.Count);
            if (func == null) return InteropBool.False;

            func.Invoke(gcHandle.Target, argsArray.ToObjectArray());

            return InteropBool.True;
        }

        internal static unsafe FieldInfo FindField(ManagedGCHandle objectHandle, string fieldName)
        {
            return objectHandle.Target.GetType()
                .GetField(fieldName, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
        }

        [UnmanagedCallersOnly]
        internal static unsafe void GetFieldValue(IntPtr objectHandle, HStringInternal* fieldNameStr, Variant* outValue)
        {
            var gcHandle = ManagedGCHandle.FromIntPtr(objectHandle);
            if (gcHandle != null && !gcHandle.IsAlive) return;

            string fieldName = NativeMarshal.HStringInternalToString(*fieldNameStr);
            var field = FindField(gcHandle, fieldName);
            if (field == null) return;

            *outValue = VariantConverter.ObjectToVariant(field.GetValue(gcHandle.Target));
        }

        [UnmanagedCallersOnly]
        internal static unsafe InteropBool SetFieldValue(IntPtr objectHandle, HStringInternal* fieldNameStr, Variant value)
        {
            var gcHandle = ManagedGCHandle.FromIntPtr(objectHandle);
            if (gcHandle != null && !gcHandle.IsAlive) return InteropBool.False;

            string fieldName = NativeMarshal.HStringInternalToString(*fieldNameStr);
            return NativeMarshal.BoolToInteropBool(
                ((Entity)gcHandle.Target).GENERATED_SetField(fieldName, value)
            );
        }
    }
}
