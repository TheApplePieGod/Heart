using Heart.Container;
using Heart.Math;
using Heart.NativeInterop;
using System;
using System.Linq;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public struct UUID
    {
        public ulong Value;

        public UUID(ulong value)
        {
            Value = value;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator UUID(ulong value)
            => new UUID(value);
    }

    public abstract class Component
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        internal Component(uint entityHandle, IntPtr sceneHandle)
        {
            _entityHandle = entityHandle;
            _sceneHandle = sceneHandle;
        }
    }

    public static class ComponentUtils
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static unsafe ContainerInfo* GetInfoFromPtr<T>(T* ptr) where T : unmanaged
            => (ContainerInfo*)ptr - 1;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe UUID GetId(uint entityHandle, IntPtr sceneHandle)
        {
            IdComponent.Native_IdComponent_Get(entityHandle, sceneHandle, out var comp);
            return *comp;
        }

        public static unsafe string GetName(uint entityHandle, IntPtr sceneHandle)
        {
            NameComponent.Native_NameComponent_Get(entityHandle, sceneHandle, out var comp);
            return NativeMarshal.HStringInternalToString(*comp);
        }

        public static void SetName(uint entityHandle, IntPtr sceneHandle, string name)
        {
            using HString hstr = new HString(name);
            NameComponent.Native_NameComponent_SetName(entityHandle, sceneHandle, hstr._internalVal);
        }

        public static unsafe UUID GetParent(uint entityHandle, IntPtr sceneHandle)
        {
            ParentComponent.Native_ParentComponent_Get(entityHandle, sceneHandle, out var uuid);
            return *uuid;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void SetParent(uint entityHandle, IntPtr sceneHandle, UUID uuid)
        {
            ParentComponent.Native_ParentComponent_SetParent(entityHandle, sceneHandle, uuid);
        }

        public static unsafe UUID GetChildId(uint entityHandle, IntPtr sceneHandle, uint index)
        {
            ChildrenComponent.Native_ChildrenComponent_Get(entityHandle, sceneHandle, out var arr);
            return arr[index];
        }

        public static unsafe Entity GetChild(uint entityHandle, IntPtr sceneHandle, uint index)
        {
            Scene.Native_Scene_GetEntityFromUUID(sceneHandle, GetChildId(entityHandle, sceneHandle, index), out var childHandle);
            return new Entity(childHandle, sceneHandle);
        }

        public static unsafe UUID[] GetChildrenIds(uint entityHandle, IntPtr sceneHandle)
        {
            ChildrenComponent.Native_ChildrenComponent_Get(entityHandle, sceneHandle, out var arr);
            return NativeMarshal.PtrToArray(arr, GetInfoFromPtr(arr)->ElemCount);
        }

        public static unsafe Entity[] GetChildren(uint entityHandle, IntPtr sceneHandle)
        {
            var ids = GetChildrenIds(entityHandle, sceneHandle);
            return ids.Select(id =>
            {
                Scene.Native_Scene_GetEntityFromUUID(sceneHandle, id, out var childHandle);
                return new Entity(childHandle, sceneHandle);
            }).ToArray();
        }

        public static unsafe uint GetChildrenCount(uint entityHandle, IntPtr sceneHandle)
        {
            ChildrenComponent.Native_ChildrenComponent_Get(entityHandle, sceneHandle, out var arr);
            return GetInfoFromPtr(arr)->ElemCount;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddChild(uint entityHandle, IntPtr sceneHandle, UUID uuid)
        {
            ChildrenComponent.Native_ChildrenComponent_AddChild(entityHandle, sceneHandle, uuid);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void RemoveChild(uint entityHandle, IntPtr sceneHandle, UUID uuid)
        {
            ChildrenComponent.Native_ChildrenComponent_RemoveChild(entityHandle, sceneHandle, uuid);
        }

        public static unsafe Vec3 GetTranslation(uint entityHandle, IntPtr sceneHandle)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            return new Vec3(comp->Translation);
        }

        public static unsafe Vec3 GetRotation(uint entityHandle, IntPtr sceneHandle)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            return new Vec3(comp->Rotation);
        }

        public static unsafe Vec3 GetScale(uint entityHandle, IntPtr sceneHandle)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            return new Vec3(comp->Scale);
        }

        public static unsafe void SetTranslation(uint entityHandle, IntPtr sceneHandle, Vec3 translation)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            comp->Translation = translation.ToVec3Internal();
            TransformComponent.Native_TransformComponent_CacheTransform(entityHandle, sceneHandle);
        }

        public static unsafe void SetRotation(uint entityHandle, IntPtr sceneHandle, Vec3 rotation)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            comp->Rotation = rotation.ToVec3Internal();
            TransformComponent.Native_TransformComponent_CacheTransform(entityHandle, sceneHandle);
        }

        public static unsafe void SetScale(uint entityHandle, IntPtr sceneHandle, Vec3 scale)
        {
            TransformComponent.Native_TransformComponent_Get(entityHandle, sceneHandle, out var comp);
            comp->Scale = scale.ToVec3Internal();
            TransformComponent.Native_TransformComponent_CacheTransform(entityHandle, sceneHandle);
        }

        public static Vec3 GetForwardVector(uint entityHandle, IntPtr sceneHandle)
        {
            TransformComponent.Native_TransformComponent_GetForwardVector(entityHandle, sceneHandle, out var value);
            return new Vec3(value);
        }

        public static T GetComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component, new()
        {
            if (!HasComponent<T>(entityHandle, sceneHandle)) return null;
            return new T { _entityHandle = entityHandle, _sceneHandle = sceneHandle };
        }

        // This is mid
        public static bool HasComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component
        {
            switch (typeof(T))
            {
                case var t when t == typeof(IdComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(NameComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(TransformComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(ParentComponent):
                    { return true; /* Will always be created if null */ }
                case var t when t == typeof(ChildrenComponent):
                    { return true; /* Will always be created if null */ }
                case var t when t == typeof(MeshComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_MeshComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(LightComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_LightComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(ScriptComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_ScriptComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(CameraComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_CameraComponent_Exists(entityHandle, sceneHandle)); }
            }

            throw new NotImplementedException("HasComponent does not support " + typeof(T).FullName);
        }

        [DllImport("__Internal")]
        internal static extern InteropBool Native_MeshComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_LightComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_ScriptComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_CameraComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
