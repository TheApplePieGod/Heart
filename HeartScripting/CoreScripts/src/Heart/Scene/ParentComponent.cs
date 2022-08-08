using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class ParentComponent : Component
    {
        public ParentComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal ParentComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        public UUID Id
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetParent(_entityHandle, _sceneHandle);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => ComponentUtils.SetParent(_entityHandle, _sceneHandle, value);
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_ParentComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);

        [DllImport("__Internal")]
        internal static extern void Native_ParentComponent_SetParent(uint entityHandle, IntPtr sceneHandle, UUID parent);
    }
}
