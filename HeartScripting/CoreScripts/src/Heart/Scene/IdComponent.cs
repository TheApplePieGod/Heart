using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class IdComponent : Component
    {
        public IdComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal IdComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        public UUID Id
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetId(_entityHandle, _sceneHandle);
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_IdComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);
    }
}
