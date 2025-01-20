using Heart.Container;
using Heart.NativeInterop;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class ChildrenComponent : IComponent
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add a children component");

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove a children component");

        [UnmanagedCallback]
        internal static unsafe partial void Native_ChildrenComponent_Get(uint entityHandle, IntPtr sceneHandle, out UUID* comp);

        [UnmanagedCallback]
        internal static partial void Native_ChildrenComponent_AddChild(uint entityHandle, IntPtr sceneHandle, UUID uuid);

        [UnmanagedCallback]
        internal static partial void Native_ChildrenComponent_RemoveChild(uint entityHandle, IntPtr sceneHandle, UUID uuid);
    }
}
