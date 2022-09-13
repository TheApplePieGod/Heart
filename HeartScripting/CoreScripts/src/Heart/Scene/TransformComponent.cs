using Heart.Math;
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

    public class TransformComponent : Component
    {
        public TransformComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal TransformComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetTranslation()
            => ComponentUtils.GetTranslation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetTranslation(Vec3 translation)
            => ComponentUtils.SetTranslation(_entityHandle, _sceneHandle, translation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetRotation()
            => ComponentUtils.GetRotation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRotation(Vec3 rotation)
            => ComponentUtils.SetRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetScale()
            => ComponentUtils.GetScale(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(Vec3 scale)
            => ComponentUtils.SetScale(_entityHandle, _sceneHandle, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetForwardVector()
            => ComponentUtils.GetForwardVector(_entityHandle, _sceneHandle);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TransformComponent_Get(uint entityHandle, IntPtr sceneHandle, out TransformComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_CacheTransform(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_GetForwardVector(uint entityHandle, IntPtr sceneHandle, out Vec3Internal value);
    }
}
