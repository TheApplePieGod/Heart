﻿using Heart.Container;
using Heart.NativeInterop;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class NameComponent : IComponent
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        public string Name
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetName(_entityHandle, _sceneHandle);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => ComponentUtils.SetName(_entityHandle, _sceneHandle, value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add a name component");

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove a name component");

        [UnmanagedCallback]
        internal static unsafe partial void Native_NameComponent_Get(uint entityHandle, IntPtr sceneHandle, out HStringInternal* comp);

        [UnmanagedCallback]
        internal static partial void Native_NameComponent_SetName(uint entityHandle, IntPtr sceneHandle, HStringInternal value);
    }
}
