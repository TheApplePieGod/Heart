using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;

namespace Heart.Scene
{
    public partial class IdComponent : IComponent<IdComponent>
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        public UUID Id
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetId(_entityHandle, _sceneHandle);
        }

        public static unsafe InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;
        public static unsafe void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add an id component");
        public static unsafe void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove an id component");

        [DllImport("__Internal")]
        internal static extern unsafe void Native_IdComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);
    }
}
