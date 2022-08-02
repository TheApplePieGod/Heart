using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class Component
    {
        internal uint _entityHandle;
        internal IntPtr _sceneHandle;

        public Component(uint entityHandle, IntPtr sceneHandle)
        {
            _entityHandle = entityHandle;
            _sceneHandle = sceneHandle;
        }
    }

    public static class ComponentUtils
    {
        // This is mid
        public static unsafe T GetComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component
        {
            switch (typeof(T))
            {
                case var t when t == typeof(TransformComponent):
                    {
                        var comp = new TransformComponent(entityHandle, sceneHandle);
                        return Unsafe.As<TransformComponent, T>(ref comp);
                    }
                case var t when t == typeof(MeshComponent):
                    {
                        var comp = new MeshComponent(entityHandle, sceneHandle);
                        return Unsafe.As<MeshComponent, T>(ref comp);
                    }
            }

            throw new NotImplementedException("GetComponent does not support " + typeof(T).FullName);
        }

        public static unsafe bool HasComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component
        {
            switch (typeof(T))
            {
                case var t when t == typeof(TransformComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(MeshComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_MeshComponent_Exists(entityHandle, sceneHandle)); }
            }

            throw new NotImplementedException("HasComponent does not support " + typeof(T).FullName);
        }

        [DllImport("__Internal")]
        internal static extern InteropBool Native_MeshComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
