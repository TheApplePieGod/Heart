using Heart.Container;
using Heart.Math;
using Heart.NativeInterop;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public enum LightType : uint
    {
        Disabled = 0,
        Directional, Point
    }

    [StructLayout(LayoutKind.Explicit, Size = 24)]
    internal unsafe struct LightComponentInternal
    {
        [FieldOffset(0)] public Vec4Internal Color;
        [FieldOffset(16)] public uint LightType;
        [FieldOffset(20)] public float Radius;
    }

    public partial class LightComponent : IComponent
    {
        internal unsafe LightComponentInternal* _internalValue;
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        private unsafe void RefreshPtr()
        {
            Native_LightComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify light component that no longer exists");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetColor()
        {
            RefreshPtr();
            return new Vec3(_internalValue->Color);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void SetColor(Vec3 color)
        {
            RefreshPtr();
            _internalValue->Color =
                color.ToVec4Internal(_internalValue->Color.W);
        }

        public unsafe LightType LightType
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return (LightType)_internalValue->LightType;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->LightType = (uint)value;
            }
        }

        public unsafe float Intensity
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Color.W;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Color.W = value;
            }
        }

        public unsafe float Radius
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Radius;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Radius = value;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_LightComponent_Exists(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_LightComponent_Add(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_LightComponent_Remove(entityHandle, sceneHandle);

        [UnmanagedCallback]
        internal static unsafe partial void Native_LightComponent_Get(uint entityHandle, IntPtr sceneHandle, out LightComponentInternal* comp);

        [UnmanagedCallback]
        internal static partial void Native_LightComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial void Native_LightComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial InteropBool Native_LightComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
