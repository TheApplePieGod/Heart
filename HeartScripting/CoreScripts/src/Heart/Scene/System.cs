using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Container;
using Heart.NativeInterop;
using Heart.Physics;

namespace Heart.Scene
{
    public class System
    {
        public System()
        {
            
        }

        public virtual void Run()
        {}


        [DllImport("__Internal")]
        internal static extern void Native_Scene_CreateEntity(IntPtr sceneHandle, [MarshalAs(UnmanagedType.LPStr)] string name, out uint entityHandle);

        [DllImport("__Internal")]
        internal static extern void Native_Scene_GetEntityFromUUID(IntPtr sceneHandle, UUID uuid, out uint entityHandle);

        [DllImport("__Internal")]
        internal static extern void Native_Scene_GetEntityFromName(IntPtr sceneHandle, [MarshalAs(UnmanagedType.LPStr)] string name, out uint entityHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_Scene_RaycastSingle(IntPtr sceneHandle, in RaycastInfoInternal info, out RaycastResultInternal outResult);
    }
}
