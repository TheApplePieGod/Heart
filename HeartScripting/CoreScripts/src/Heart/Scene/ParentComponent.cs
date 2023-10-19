using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class ParentComponent : IComponent<ParentComponent>
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        public UUID Id
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetParent(_entityHandle, _sceneHandle);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => ComponentUtils.SetParent(_entityHandle, _sceneHandle, value);
        }

        public static unsafe InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;
        public static unsafe void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add a parent component");
        public static unsafe void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove a parent component");

        [DllImport("__Internal")]
        internal static extern unsafe void Native_ParentComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);

        [DllImport("__Internal")]
        internal static extern void Native_ParentComponent_SetParent(uint entityHandle, IntPtr sceneHandle, UUID parent);
    }
}
