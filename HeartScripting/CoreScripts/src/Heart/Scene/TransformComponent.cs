using Heart.Math;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 36)]
    internal struct TransformComponentInternal
    {
        [FieldOffset(0)] public Vec3Internal Translation;
        [FieldOffset(12)] public Vec3Internal Rotation;
        [FieldOffset(24)] public Vec3Internal Scale;
    }

    public partial class TransformComponent : IComponent
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetPosition()
            => ComponentUtils.GetPosition(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetPosition(Vec3 position)
            => ComponentUtils.SetPosition(_entityHandle, _sceneHandle, position);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetRotation()
            => ComponentUtils.GetRotation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRotation(Vec3 rotation)
            => ComponentUtils.SetRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void ApplyRotation(Vec3 rotation)
            => ComponentUtils.ApplyRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetScale()
            => ComponentUtils.GetScale(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(Vec3 scale)
            => ComponentUtils.SetScale(_entityHandle, _sceneHandle, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetTransform(Vec3 pos, Vec3 rot, Vec3 scale)
            => ComponentUtils.SetTransform(_entityHandle, _sceneHandle, pos, rot, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetForwardVector()
            => ComponentUtils.GetForwardVector(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => InteropBool.True;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot add a transform component");

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => throw new InvalidOperationException("Cannot remove a transform component");

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TransformComponent_Get(uint entityHandle, IntPtr sceneHandle, out TransformComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_SetPosition(uint entityHandle, IntPtr sceneHandle, Vec3Internal value);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_SetRotation(uint entityHandle, IntPtr sceneHandle, Vec3Internal value);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_SetScale(uint entityHandle, IntPtr sceneHandle, Vec3Internal value);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_SetTransform(uint entityHandle, IntPtr sceneHandle, Vec3Internal pos, Vec3Internal rot, Vec3Internal scale);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_ApplyRotation(uint entityHandle, IntPtr sceneHandle, Vec3Internal value);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_GetForwardVector(uint entityHandle, IntPtr sceneHandle, out Vec3Internal value);
    }
}
