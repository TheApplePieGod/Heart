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

    public partial class CameraComponent : IComponent
    {
        internal unsafe CameraComponentInternal* _internalValue;
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_CameraComponent_Exists(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_CameraComponent_Add(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_CameraComponent_Remove(entityHandle, sceneHandle);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_CameraComponent_Get(uint entityHandle, IntPtr sceneHandle, out CameraComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_CameraComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_CameraComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_CameraComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_CameraComponent_SetPrimary(uint entityHandle, IntPtr sceneHandle, InteropBool primary);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_PrimaryCameraComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
