using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class Scene
    {
        internal IntPtr _internalValue;

        public Scene(Entity entity)
        {
            _internalValue = entity._sceneHandle;
        }

        internal Scene(IntPtr sceneHandle)
        {
            _internalValue = sceneHandle;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity CreateEntity(string name = "New Entity")
            => CreateEntity(_internalValue, name);

        internal static Entity CreateEntity(IntPtr sceneHandle, string name = "New Entity")
        {
            using HString hstr = new HString(name);
            Native_Scene_CreateEntity(sceneHandle, hstr._internalVal, out var entityHandle);
            return new Entity(entityHandle, sceneHandle);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity GetEntityFromUUID(UUID uuid)
            => GetEntityFromUUID(_internalValue, uuid);

        internal static Entity GetEntityFromUUID(IntPtr sceneHandle, UUID uuid)
        {
            Native_Scene_GetEntityFromUUID(sceneHandle, uuid, out var entityHandle);
            if (entityHandle == Entity.InvalidEntityHandle) return null;
            return new Entity(entityHandle, sceneHandle);
        }

        [DllImport("__Internal")]
        internal static extern void Native_Scene_CreateEntity(IntPtr sceneHandle, in HStringInternal name, out uint entityHandle);

        [DllImport("__Internal")]
        internal static extern void Native_Scene_GetEntityFromUUID(IntPtr sceneHandle, UUID uuid, out uint entityHandle);
    }
}
