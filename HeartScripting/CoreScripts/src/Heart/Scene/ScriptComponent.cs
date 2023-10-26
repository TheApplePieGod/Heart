using Heart.Container;
using Heart.Math;
using Heart.NativeBridge;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class ScriptComponent : IComponent
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void InstantiateScript()
            => Native_ScriptComponent_InstantiateScript(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void DestroyScript()
            => Native_ScriptComponent_DestroyScript(_entityHandle, _sceneHandle);

        public unsafe string ScriptClass
        {
            get
            {
                Native_ScriptComponent_GetScriptClass(_entityHandle, _sceneHandle, out var value);
                return NativeMarshal.HStringInternalToString(*value);
            }
            set
            {
                Native_ScriptComponent_SetScriptClass(_entityHandle, _sceneHandle, value);
            }
        }

        public unsafe bool IsAlive
        {
            get
            {
                Native_ScriptComponent_GetObjectHandle(_entityHandle, _sceneHandle, out var handle);
                return handle != IntPtr.Zero;
            }
        }

        public bool IsInstantiable
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ScriptClass.Length != 0;
        }

        public unsafe ScriptEntity ScriptObject
        {
            get
            {
                Native_ScriptComponent_GetObjectHandle(_entityHandle, _sceneHandle, out var handle);
                if (handle == IntPtr.Zero) return null;
                return (ScriptEntity)ManagedGCHandle.FromIntPtr(handle).Target;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_ScriptComponent_Exists(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_ScriptComponent_Add(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_ScriptComponent_Remove(entityHandle, sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_ScriptComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_ScriptComponent_GetObjectHandle(uint entityHandle, IntPtr sceneHandle, out IntPtr objectHandle);
        
        [DllImport("__Internal")]
        internal static extern unsafe void Native_ScriptComponent_GetScriptClass(uint entityHandle, IntPtr sceneHandle, out HStringInternal* value);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_SetScriptClass(uint entityHandle, IntPtr sceneHandle, [MarshalAs(UnmanagedType.LPStr)] string value);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_InstantiateScript(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_ScriptComponent_DestroyScript(uint entityHandle, IntPtr sceneHandle);
    }
}
