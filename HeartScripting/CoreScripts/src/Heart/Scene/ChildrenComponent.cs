﻿using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class ChildrenComponent : Component
    {
        public ChildrenComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal ChildrenComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        { }

        public uint Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ComponentUtils.GetChildrenCount(_entityHandle, _sceneHandle);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID GetChildId(uint index)
            => ComponentUtils.GetChildId(_entityHandle, _sceneHandle, index);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity GetChild(uint index)
            => ComponentUtils.GetChild(_entityHandle, _sceneHandle, index);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID[] GetChildrenIds()
            => ComponentUtils.GetChildrenIds(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity[] GetChildren()
            => ComponentUtils.GetChildren(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void AddChild(UUID uuid)
            => ComponentUtils.AddChild(_entityHandle, _sceneHandle, uuid);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity CreateChild(string name)
            => ComponentUtils.CreateChild(_entityHandle, _sceneHandle, name);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void RemoveChild(UUID uuid)
            => ComponentUtils.RemoveChild(_entityHandle, _sceneHandle, uuid);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_ChildrenComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);

        [DllImport("__Internal")]
        internal static extern void Native_ChildrenComponent_AddChild(uint entityHandle, IntPtr sceneHandle, UUID uuid);

        [DllImport("__Internal")]
        internal static extern void Native_ChildrenComponent_RemoveChild(uint entityHandle, IntPtr sceneHandle, UUID uuid);
    }
}
