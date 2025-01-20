using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;
using Heart.NativeBridge;

namespace Heart.Scene
{
    // Static helpers to be utilized in generated component code
    public static partial class RuntimeComponent
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle, Int64 uniqueId)
            => Native_RuntimeComponent_Exists(entityHandle, sceneHandle, uniqueId);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle, Int64 uniqueId)
            => Native_RuntimeComponent_Remove(entityHandle, sceneHandle, uniqueId);

        public static void NativeAdd<T>(uint entityHandle, IntPtr sceneHandle, Int64 uniqueId) where T : class, new()
        {
            var instance = new T();
            var handle = ManagedGCHandle.AllocStrong(instance);
            IntPtr objectHandle = handle.ToIntPtr();
            Native_RuntimeComponent_Add(entityHandle, sceneHandle, uniqueId, objectHandle);
        }

        public static T Create<T>(uint entityHandle, IntPtr sceneHandle, Int64 uniqueId)
        {
            Native_RuntimeComponent_Get(entityHandle, sceneHandle, uniqueId, out IntPtr objectHandle);
            var gcHandle = ManagedGCHandle.FromIntPtr(objectHandle);
            if (gcHandle != null && !gcHandle.IsAlive)
                throw new InvalidOperationException($"Runtime component object is invalid (id: {uniqueId})");
            return (T)gcHandle.Target;
        }

        [UnmanagedCallback]
        internal static partial InteropBool Native_RuntimeComponent_Exists(uint entityHandle, IntPtr sceneHandle, Int64 typeId);

        [UnmanagedCallback]
        internal static partial InteropBool Native_RuntimeComponent_Add(uint entityHandle, IntPtr sceneHandle, Int64 typeId, IntPtr objectHandle);

        [UnmanagedCallback]
        internal static partial InteropBool Native_RuntimeComponent_Remove(uint entityHandle, IntPtr sceneHandle, Int64 typeId);

        [UnmanagedCallback]
        internal static partial InteropBool Native_RuntimeComponent_Get(uint entityHandle, IntPtr sceneHandle, Int64 typeId, out IntPtr comp);
    }
}
