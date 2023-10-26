using System;
using System.Linq;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Core;
using Heart.NativeInterop;
using Heart.Physics;
using Heart.Task;

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
            Native_Scene_CreateEntity(sceneHandle, name, out var entityHandle);
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

        public Entity GetEntityFromName(string name)
        {
            Native_Scene_GetEntityFromName(_internalValue, name, out var entityHandle);
            if (entityHandle == Entity.InvalidEntityHandle) return null;
            return new Entity(entityHandle, _internalValue);
        }
        
        public bool RaycastSingle(RaycastInfo castInfo, out RaycastResult outResult)
        {
            var success = Native_Scene_RaycastSingle(_internalValue, castInfo._internal, out var res);
            outResult = new RaycastResult(res);
            return NativeMarshal.InteropBoolToBool(success);
        }

        public ISchedulable CreateEntityIterator(Func<Entity, List<Action>> func)
        {
            ConcurrentBag<Action> transactions = new();
            var view = new EntityView(this);
            return new SchedulableIter(
                view.Select(entity => (nuint)entity._entityHandle),
                (nuint val) =>
                {
                    try
                    {
                        var result = func(new Entity((uint)val, _internalValue));
                        if (result != null)
                            foreach (var elem in result)
                                transactions.Add(elem);
                    }
                    catch (Exception e)
                    {
                        Log.Error("EntityIterator execution threw an exception: {0}", e.Message);
                    }
                },
                null,
                () =>
                {
                    foreach (var t in transactions)
                        t();
                }
            );
        }

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
