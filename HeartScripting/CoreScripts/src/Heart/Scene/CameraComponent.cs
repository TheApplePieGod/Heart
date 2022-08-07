using Heart.Container;
using Heart.Math;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 12)]
    internal unsafe struct CameraComponentInternal
    {
        [FieldOffset(0)] public float FOV;
        [FieldOffset(4)] public float NearClipPlane;
        [FieldOffset(8)] public float FarClipPlane;
    }

    public class CameraComponent : Component
    {
        internal unsafe CameraComponentInternal* _internalValue;

        public CameraComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }
        internal CameraComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        { }

        private unsafe void RefreshPtr()
        {
            Native_CameraComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify camera component that no longer exists");
        }

        public unsafe float FOV
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->FOV;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->FOV = value;
            }
        }

        public unsafe float NearClipPlane
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->NearClipPlane;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->NearClipPlane = value;
            }
        }

        public unsafe float FarClipPlane
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->FarClipPlane;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->FarClipPlane = value;
            }
        }

        public bool IsPrimary
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => NativeMarshal.InteropBoolToBool(Native_PrimaryCameraComponent_Exists(_entityHandle, _sceneHandle));

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => Native_CameraComponent_SetPrimary(_entityHandle, _sceneHandle, NativeMarshal.BoolToInteropBool(value));
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_CameraComponent_Get(uint entityHandle, IntPtr sceneHandle, out CameraComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_CameraComponent_SetPrimary(uint entityHandle, IntPtr sceneHandle, InteropBool primary);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_PrimaryCameraComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
