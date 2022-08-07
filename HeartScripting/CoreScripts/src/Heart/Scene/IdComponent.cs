using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class IdComponent : Component
    {
        internal unsafe UUID* _internalValue;

        public IdComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal IdComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private unsafe void RefreshPtr()
        {
            // We shouldn't need safety checking here because all entities are guaranteed
            // to have an id comp
            Native_IdComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
        }

        public unsafe UUID Id
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return *_internalValue;
            }
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_IdComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);
    }
}
