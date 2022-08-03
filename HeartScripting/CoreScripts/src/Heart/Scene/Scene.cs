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

        public Entity CreateEntity(string name = "New Entity")
        {
            using HString hstr = new HString(name);
            Native_Scene_CreateEntity(_internalValue, hstr._internalVal, out var entityHandle);
            return new Entity(entityHandle, _internalValue);
        }

        public Entity GetEntityFromUUID(UUID uuid)
        {
            Native_Scene_GetEntityFromUUID(_internalValue, uuid, out var entityHandle);
            if (entityHandle == Entity.InvalidEntityHandle) return null;
            return new Entity(entityHandle, _internalValue);
        }

        [DllImport("__Internal")]
        internal static extern void Native_Scene_CreateEntity(IntPtr sceneHandle, HStringInternal name, out uint entityHandle);

        [DllImport("__Internal")]
        internal static extern void Native_Scene_GetEntityFromUUID(IntPtr sceneHandle, UUID uuid, out uint entityHandle);
    }
}
