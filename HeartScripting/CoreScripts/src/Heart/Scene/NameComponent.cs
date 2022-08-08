using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class NameComponent : Component
    {
        public NameComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal NameComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        public string Name
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetName(_entityHandle, _sceneHandle);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => ComponentUtils.SetName(_entityHandle, _sceneHandle, value);
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_NameComponent_Get(uint entityHandle, IntPtr sceneHandle, out HStringInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_NameComponent_SetName(uint entityHandle, IntPtr sceneHandle, HStringInternal value);
    }
}
