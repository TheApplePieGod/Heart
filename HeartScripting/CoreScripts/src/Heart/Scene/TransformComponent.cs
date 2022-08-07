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
        internal unsafe TransformComponentInternal* _internalValue;

        public TransformComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal TransformComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private unsafe void RefreshPtr()
        {
            // We shouldn't need safety checking here because all entities are guaranteed
            // to have a transform comp
            Native_TransformComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void CacheTransform()
            => Native_TransformComponent_CacheTransform(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetTranslation()
        {
            RefreshPtr();
            return new Vec3(_internalValue->Translation);
        }

        public unsafe void SetTranslation(Vec3 translation)
        {
            RefreshPtr();
            _internalValue->Translation = translation.ToVec3Internal();
            CacheTransform();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetRotation()
        {
            RefreshPtr();
            return new Vec3(_internalValue->Rotation);
        }

        public unsafe void SetRotation(Vec3 rotation)
        {
            RefreshPtr();
            _internalValue->Rotation = rotation.ToVec3Internal();
            CacheTransform();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetScale()
        {
            RefreshPtr();
            return new Vec3(_internalValue->Scale);
        }

        public unsafe void SetScale(Vec3 scale)
        {
            RefreshPtr();
            _internalValue->Scale = scale.ToVec3Internal();
            CacheTransform();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetForwardVector()
        {
            Native_TransformComponent_GetForwardVector(_entityHandle, _sceneHandle, out var value);
            return new Vec3(value);
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TransformComponent_Get(uint entityHandle, IntPtr sceneHandle, out TransformComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_CacheTransform(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_TransformComponent_GetForwardVector(uint entityHandle, IntPtr sceneHandle, out Vec3Internal value);
    }
}
