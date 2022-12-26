using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 4)]
    internal unsafe struct RigidBodyComponentInternal
    {
        [FieldOffset(0)] public uint BodyId;
    }

    public class RigidBodyComponent : Component
    {
        public RigidBodyComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal RigidBodyComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        [DllImport("__Internal")]
        internal static extern unsafe void Native_RigidBodyComponent_Get(uint entityHandle, IntPtr sceneHandle, out RigidBodyComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_RigidBodyComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
