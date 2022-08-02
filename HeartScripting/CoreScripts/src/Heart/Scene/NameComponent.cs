using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class NameComponent : Component
    {
        internal unsafe HStringInternal* _internalValue;

        internal NameComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private unsafe void RefreshPtr()
        {
            // We shouldn't need safety checking here because all entities are guaranteed
            // to have a name comp
            Native_NameComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
        }

        public unsafe string Name
        {
            get
            {
                RefreshPtr();
                return NativeMarshal.HStringInternalToString(*_internalValue);
            }
            set
            {
                RefreshPtr();
                using HString hstr = new HString(value);
                Native_NameComponent_SetName(_entityHandle, _sceneHandle, hstr._internalVal);
            }
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_NameComponent_Get(uint entityHandle, IntPtr sceneHandle, out HStringInternal* comp);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_NameComponent_SetName(uint entityHandle, IntPtr sceneHandle, HStringInternal value);
    }
}
