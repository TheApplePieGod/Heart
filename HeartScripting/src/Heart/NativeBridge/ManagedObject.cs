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
        public static IntPtr InstantiateClientObject([In] IntPtr objectTypeStr)
        {
            if (EntryPoint.ClientAssembly == null) return IntPtr.Zero;

            string typeStr = Marshal.PtrToStringUTF8(objectTypeStr);
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
    }
}
