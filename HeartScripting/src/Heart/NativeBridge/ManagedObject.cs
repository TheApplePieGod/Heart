using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace Heart.NativeBridge
{
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

        [UnmanagedCallersOnly]
        internal static unsafe InteropBool InvokeFunction(IntPtr objectHandle, HStringInternal* funcNameStr, HArrayInternal* args)
        {
            if (objectHandle == IntPtr.Zero) return InteropBool.False;

            var gcHandle = ManagedGCHandle.FromIntPtr(objectHandle);
            if (!gcHandle.IsAlive) return InteropBool.False;

            HArray argsArray = new HArray(*args);

            string funcName = NativeMarshal.HStringInternalToString(*funcNameStr);
            var func = gcHandle.Target.GetType()
                .GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)
                .Where(m => m.Name == funcName && m.GetParameters().Length == argsArray.Count)
                .FirstOrDefault();
            if (func == null) return InteropBool.False;

            func.Invoke(gcHandle.Target, argsArray.ToObjectArray());

            return InteropBool.True;
        }
    }
}
