using Heart.Container;
using Heart.Math;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public enum LightType : byte
    {
        Disabled = 0,
        Directional, Point
    }

    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal unsafe struct LightComponentInternal
    {
        [FieldOffset(0)] public LightType LightType;
        [FieldOffset(8)] public Vec4Internal Color;
        [FieldOffset(24)] public float ConstantAttenuation;
        [FieldOffset(28)] public float LinearAttenuation;
        [FieldOffset(32)] public float QuadraticAttenuation;
    }

    public class LightComponent : Component
    {
        internal unsafe LightComponentInternal* _internalValue;

        internal LightComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        { }

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
                return _internalValue->LightType;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->LightType = value;
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

        public unsafe float ConstantAttenuation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->ConstantAttenuation;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->ConstantAttenuation = value;
            }
        }

        public unsafe float LinearAttenuation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->LinearAttenuation;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->LinearAttenuation = value;
            }
        }

        public unsafe float QuadraticAttenuation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->QuadraticAttenuation;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->QuadraticAttenuation = value;
            }
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_LightComponent_Get(uint entityHandle, IntPtr sceneHandle, out LightComponentInternal* comp);
    }
}
