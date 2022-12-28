using Heart.Container;
using Heart.Math;
using Heart.NativeBridge;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 24)]
    internal struct ScriptInstanceInternal
    {
        [FieldOffset(0)] public IntPtr ObjectHandle;
        [FieldOffset(8)] public HStringInternal ScriptClass;
    }

    [StructLayout(LayoutKind.Explicit, Size = 24)]
    internal struct ScriptComponentInternal
    {
        [FieldOffset(0)] public ScriptInstanceInternal ScriptInstance;
    }

    public class ScriptComponent : Component
    {
        internal unsafe ScriptComponentInternal* _internalValue;

        public ScriptComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal ScriptComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        { }

        private unsafe void RefreshPtr()
        {
            Native_ScriptComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify script component that no longer exists");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void InstantiateScript()
            => Native_ScriptComponent_InstantiateScript(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void DestroyScript()
            => Native_ScriptComponent_DestroyScript(_entityHandle, _sceneHandle);

        public unsafe ScriptEntity ScriptObject
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                if (!IsAlive) return null;
                return (ScriptEntity)ManagedGCHandle.FromIntPtr(_internalValue->ScriptInstance.ObjectHandle).Target;
            }
        }

        public unsafe string ScriptClass
        {
            get
            {
                RefreshPtr();
                return NativeMarshal.HStringInternalToString(_internalValue->ScriptInstance.ScriptClass);
            }
            set
            {
                RefreshPtr();
                Native_ScriptComponent_SetScriptClass(_entityHandle, _sceneHandle, value);
            }
        }

        public unsafe bool IsAlive
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->ScriptInstance.ObjectHandle != IntPtr.Zero;
            }
        }

        public bool IsInstantiable
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ScriptClass.Length != 0;
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_ScriptComponent_Get(uint entityHandle, IntPtr sceneHandle, out ScriptComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_ScriptComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_SetScriptClass(uint entityHandle, IntPtr sceneHandle, [MarshalAs(UnmanagedType.LPStr)] string value);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_InstantiateScript(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_DestroyScript(uint entityHandle, IntPtr sceneHandle);
    }
}
