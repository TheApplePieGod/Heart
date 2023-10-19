using Heart.Container;
using Heart.NativeInterop;
using Heart.Math;
using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 64)]
    internal unsafe struct TextComponentInternal
    {
        [FieldOffset(0)] public UUID Font;
        [FieldOffset(8)] public HStringInternal Text;
        [FieldOffset(24)] public float FontSize;
        [FieldOffset(28)] public float LineHeight;
        [FieldOffset(32)] public Vec3Internal BaseColor;
        [FieldOffset(44)] public Vec3Internal EmissiveFactor;
        [FieldOffset(56)] public float Metalness;
        [FieldOffset(60)] public float Roughness;
    }

    public partial class TextComponent : IComponent<TextComponent>
    {
        internal unsafe TextComponentInternal* _internalValue;
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        private unsafe void RefreshPtr()
        {
            Native_TextComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify text component that no longer exists");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetBaseColor()
        {
            RefreshPtr();
            return new Vec3(_internalValue->BaseColor);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void SetBaseColor(Vec3 color)
        {
            RefreshPtr();
            _internalValue->BaseColor = color._internal;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe Vec3 GetEmissiveFactor()
        {
            RefreshPtr();
            return new Vec3(_internalValue->EmissiveFactor);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void SetEmissiveFactor(Vec3 factor)
        {
            RefreshPtr();
            _internalValue->EmissiveFactor = factor._internal;
        }

        public unsafe UUID Font
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Font;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Font = value;
            }
        }

        public unsafe string Text
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return NativeMarshal.HStringInternalToString(_internalValue->Text);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                Native_TextComponent_SetText(_entityHandle, _sceneHandle, value);
            }
        }

        public unsafe float FontSize
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->FontSize;
            }

            set
            {
                RefreshPtr();
                _internalValue->FontSize = value;
                Native_TextComponent_ClearRenderData(_entityHandle, _sceneHandle);
            }
        }

        public unsafe float LineHeight
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->LineHeight;
            }

            set
            {
                RefreshPtr();
                _internalValue->LineHeight = value;
                Native_TextComponent_ClearRenderData(_entityHandle, _sceneHandle);
            }
        }

        public unsafe float Metalness
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Metalness;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Metalness = value;
            }
        }

        public unsafe float Roughness
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Roughness;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Roughness = value;
            }
        }

        public static unsafe InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_TextComponent_Exists(entityHandle, sceneHandle);
        public static unsafe void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_TextComponent_Add(entityHandle, sceneHandle);
        public static unsafe void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_TextComponent_Remove(entityHandle, sceneHandle);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TextComponent_Get(uint entityHandle, IntPtr sceneHandle, out TextComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_TextComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_TextComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_TextComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_TextComponent_SetText(uint entityHandle, IntPtr sceneHandle, [MarshalAs(UnmanagedType.LPStr)] string text);

        [DllImport("__Internal")]
        internal static extern void Native_TextComponent_ClearRenderData(uint entityHandle, IntPtr sceneHandle);
    }
}
