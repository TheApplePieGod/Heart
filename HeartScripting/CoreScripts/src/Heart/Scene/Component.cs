﻿using Heart.Container;
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator ulong(UUID value)
            => value.Value;
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
            if (arr == null) return 0;
            return arr[index];
        }

        public static unsafe Entity GetChild(uint entityHandle, IntPtr sceneHandle, uint index)
        {
            UUID childId = GetChildId(entityHandle, sceneHandle, index);
            if (childId == 0) return new Entity();
            Scene.Native_Scene_GetEntityFromUUID(sceneHandle, childId, out var childHandle);
            return new Entity(childHandle, sceneHandle);
        }

        public static unsafe UUID[] GetChildrenIds(uint entityHandle, IntPtr sceneHandle)
        {
            ChildrenComponent.Native_ChildrenComponent_Get(entityHandle, sceneHandle, out var arr);
            if (arr == null) return new UUID[0];
            return NativeMarshal.PtrToArray(arr, GetInfoFromPtr(arr)->ElemCount);
        }

        public static unsafe Entity[] GetChildren(uint entityHandle, IntPtr sceneHandle)
        {
            var ids = GetChildrenIds(entityHandle, sceneHandle);
            return ids.Select(id => Scene.GetEntityFromUUID(sceneHandle, id)).ToArray();
        }

        public static unsafe uint GetChildrenCount(uint entityHandle, IntPtr sceneHandle)
        {
            ChildrenComponent.Native_ChildrenComponent_Get(entityHandle, sceneHandle, out var arr);
            if (arr == null) return 0;
            return GetInfoFromPtr(arr)->ElemCount;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddChild(uint entityHandle, IntPtr sceneHandle, UUID uuid)
        {
            ChildrenComponent.Native_ChildrenComponent_AddChild(entityHandle, sceneHandle, uuid);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Entity CreateChild(uint entityHandle, IntPtr sceneHandle, string name)
        {
            var child = Scene.CreateEntity(sceneHandle, name);
            ChildrenComponent.Native_ChildrenComponent_AddChild(entityHandle, sceneHandle, child.GetId());
            return child;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void RemoveChild(uint entityHandle, IntPtr sceneHandle, UUID uuid)
        {
            ChildrenComponent.Native_ChildrenComponent_RemoveChild(entityHandle, sceneHandle, uuid);
        }

        public static unsafe Vec3 GetPosition(uint entityHandle, IntPtr sceneHandle)
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SetPosition(uint entityHandle, IntPtr sceneHandle, Vec3 position)
        {
            TransformComponent.Native_TransformComponent_SetPosition(
                entityHandle,
                sceneHandle,
                position._internal
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SetRotation(uint entityHandle, IntPtr sceneHandle, Vec3 rotation)
        {
            TransformComponent.Native_TransformComponent_SetRotation(
                entityHandle,
                sceneHandle,
                rotation._internal
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void ApplyRotation(uint entityHandle, IntPtr sceneHandle, Vec3 rotation)
        {
            TransformComponent.Native_TransformComponent_ApplyRotation(
                entityHandle,
                sceneHandle,
                rotation._internal
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SetScale(uint entityHandle, IntPtr sceneHandle, Vec3 scale)
        {
            TransformComponent.Native_TransformComponent_SetScale(
                entityHandle,
                sceneHandle,
                scale._internal
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe void SetTransform(uint entityHandle, IntPtr sceneHandle, Vec3 pos, Vec3 rot, Vec3 scale)
        {
            TransformComponent.Native_TransformComponent_SetTransform(
                entityHandle,
                sceneHandle,
                pos._internal,
                rot._internal,
                scale._internal
            );
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
        public static bool HasComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component, new()
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
                    { return NativeMarshal.InteropBoolToBool(MeshComponent.Native_MeshComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(LightComponent):
                    { return NativeMarshal.InteropBoolToBool(LightComponent.Native_LightComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(ScriptComponent):
                    { return NativeMarshal.InteropBoolToBool(ScriptComponent.Native_ScriptComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(CameraComponent):
                    { return NativeMarshal.InteropBoolToBool(CameraComponent.Native_CameraComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(CollisionComponent):
                    { return NativeMarshal.InteropBoolToBool(CollisionComponent.Native_CollisionComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(TextComponent):
                    { return NativeMarshal.InteropBoolToBool(TextComponent.Native_TextComponent_Exists(entityHandle, sceneHandle)); }
            }

            throw new NotImplementedException("HasComponent does not support " + typeof(T).FullName);
        }

        // This is mid
        public static T AddComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component, new()
        {
            switch (typeof(T))
            {
                default:
                    { throw new NotImplementedException("AddComponent does not support " + typeof(T).FullName); }
                case var t when t == typeof(IdComponent):
                    { throw new InvalidOperationException("Cannot add an id component"); }
                case var t when t == typeof(NameComponent):
                    { throw new InvalidOperationException("Cannot add a name component"); }
                case var t when t == typeof(TransformComponent):
                    { throw new InvalidOperationException("Cannot add a transform component"); }
                case var t when t == typeof(ParentComponent):
                    { throw new InvalidOperationException("Cannot add a parent component"); }
                case var t when t == typeof(ChildrenComponent):
                    { throw new InvalidOperationException("Cannot add a children component"); }
                case var t when t == typeof(MeshComponent):
                    { MeshComponent.Native_MeshComponent_Add(entityHandle, sceneHandle); }
                    break;
                case var t when t == typeof(LightComponent):
                    { LightComponent.Native_LightComponent_Add(entityHandle, sceneHandle); }
                    break;
                case var t when t == typeof(ScriptComponent):
                    { ScriptComponent.Native_ScriptComponent_Add(entityHandle, sceneHandle); }
                    break;
                case var t when t == typeof(CameraComponent):
                    { CameraComponent.Native_CameraComponent_Add(entityHandle, sceneHandle); }
                    break;
                case var t when t == typeof(CollisionComponent):
                    { CollisionComponent.Native_CollisionComponent_Add(entityHandle, sceneHandle); }
                    break;
                case var t when t == typeof(TextComponent):
                    { TextComponent.Native_TextComponent_Add(entityHandle, sceneHandle); }
                    break;
            }

            return new T { _entityHandle = entityHandle, _sceneHandle = sceneHandle };
        }

        // This is mid
        public static void RemoveComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component, new()
        {
            switch (typeof(T))
            {
                case var t when t == typeof(IdComponent):
                    { throw new InvalidOperationException("Cannot remove an id component"); }
                case var t when t == typeof(NameComponent):
                    { throw new InvalidOperationException("Cannot remove a name component"); }
                case var t when t == typeof(TransformComponent):
                    { throw new InvalidOperationException("Cannot remove a transform component"); }
                case var t when t == typeof(ParentComponent):
                    { throw new InvalidOperationException("Cannot remove a parent component"); }
                case var t when t == typeof(ChildrenComponent):
                    { throw new InvalidOperationException("Cannot remove a children component"); }
                case var t when t == typeof(MeshComponent):
                    { MeshComponent.Native_MeshComponent_Remove(entityHandle, sceneHandle); }
                    return;
                case var t when t == typeof(LightComponent):
                    { LightComponent.Native_LightComponent_Remove(entityHandle, sceneHandle); }
                    return;
                case var t when t == typeof(ScriptComponent):
                    { ScriptComponent.Native_ScriptComponent_Remove(entityHandle, sceneHandle); }
                    return;
                case var t when t == typeof(CameraComponent):
                    { CameraComponent.Native_CameraComponent_Remove(entityHandle, sceneHandle); }
                    return;
                case var t when t == typeof(CollisionComponent):
                    { CollisionComponent.Native_CollisionComponent_Remove(entityHandle, sceneHandle); }
                    return;
                case var t when t == typeof(TextComponent):
                    { TextComponent.Native_TextComponent_Remove(entityHandle, sceneHandle); }
                    return;
            }

            throw new NotImplementedException("RemoveComponent does not support " + typeof(T).FullName);
        }
    }
}
