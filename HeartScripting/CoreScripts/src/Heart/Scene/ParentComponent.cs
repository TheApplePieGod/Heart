﻿using Heart.NativeInterop;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class ParentComponent : IComponent
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add a parent component");

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove a parent component");

        [UnmanagedCallback]
        internal static unsafe partial void Native_ParentComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);

        [UnmanagedCallback]
        internal static partial void Native_ParentComponent_SetParent(uint entityHandle, IntPtr sceneHandle, UUID parent);
    }
}
