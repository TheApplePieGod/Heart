using System;
using System.Linq;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Core;
using Heart.NativeInterop;
using Heart.NativeBridge;
using Heart.Physics;
using Heart.Task;

namespace Heart.Scene
{
    public partial class Scene
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

        internal static unsafe Entity CreateEntity(IntPtr sceneHandle, string name = "New Entity")
        {
            uint entityHandle = 0;

            fixed (char* ptr = name)
            {
                Native_Scene_CreateEntity(sceneHandle, ptr, (uint)name.Length, out entityHandle);
            }

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

        public unsafe Entity GetEntityFromName(string name)
        {
            uint entityHandle = 0;

            fixed (char* ptr = name)
            {
                Native_Scene_GetEntityFromName(_internalValue, ptr, (uint)name.Length, out entityHandle);
            }

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

        [UnmanagedCallback]
        internal static unsafe partial void Native_Scene_CreateEntity(IntPtr sceneHandle, char* name, uint nameLen, out uint entityHandle);

        [UnmanagedCallback]
        internal static partial void Native_Scene_GetEntityFromUUID(IntPtr sceneHandle, UUID uuid, out uint entityHandle);

        [UnmanagedCallback]
        internal static unsafe partial void Native_Scene_GetEntityFromName(IntPtr sceneHandle, char* name, uint nameLen, out uint entityHandle);

        [UnmanagedCallback]
        internal static partial InteropBool Native_Scene_RaycastSingle(IntPtr sceneHandle, in RaycastInfoInternal info, out RaycastResultInternal outResult);
    }
}
