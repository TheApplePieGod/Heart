using Heart.Container;
using Heart.Math;
using Heart.NativeInterop;
using System;
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

    public class Component
    {
        internal uint _entityHandle;
        internal IntPtr _sceneHandle;

        public Component(uint entityHandle, IntPtr sceneHandle)
        {
            _entityHandle = entityHandle;
            _sceneHandle = sceneHandle;
        }
    }

    public static class ComponentUtils
    {
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

        public static unsafe void SetName(uint entityHandle, IntPtr sceneHandle, string name)
        {
            using HString hstr = new HString(name);
            NameComponent.Native_NameComponent_SetName(entityHandle, sceneHandle, hstr._internalVal);
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

        // This is mid
        public static unsafe T GetComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component
        {
            switch (typeof(T))
            {
                case var t when t == typeof(IdComponent):
                    {
                        var comp = new IdComponent(entityHandle, sceneHandle);
                        return Unsafe.As<IdComponent, T>(ref comp);
                    }
                case var t when t == typeof(NameComponent):
                    {
                        var comp = new NameComponent(entityHandle, sceneHandle);
                        return Unsafe.As<NameComponent, T>(ref comp);
                    }
                case var t when t == typeof(TransformComponent):
                    {
                        var comp = new TransformComponent(entityHandle, sceneHandle);
                        return Unsafe.As<TransformComponent, T>(ref comp);
                    }
                case var t when t == typeof(MeshComponent):
                    {
                        var comp = new MeshComponent(entityHandle, sceneHandle);
                        return Unsafe.As<MeshComponent, T>(ref comp);
                    }
                case var t when t == typeof(LightComponent):
                    {
                        var comp = new LightComponent(entityHandle, sceneHandle);
                        return Unsafe.As<LightComponent, T>(ref comp);
                    }
                case var t when t == typeof(ScriptComponent):
                    {
                        var comp = new ScriptComponent(entityHandle, sceneHandle);
                        return Unsafe.As<ScriptComponent, T>(ref comp);
                    }
            }

            throw new NotImplementedException("GetComponent does not support " + typeof(T).FullName);
        }

        public static unsafe bool HasComponent<T>(uint entityHandle, IntPtr sceneHandle) where T : Component
        {
            switch (typeof(T))
            {
                case var t when t == typeof(IdComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(NameComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(TransformComponent):
                    { return true; /* Always exists */ }
                case var t when t == typeof(MeshComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_MeshComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(LightComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_LightComponent_Exists(entityHandle, sceneHandle)); }
                case var t when t == typeof(ScriptComponent):
                    { return NativeMarshal.InteropBoolToBool(Native_ScriptComponent_Exists(entityHandle, sceneHandle)); }
            }

            throw new NotImplementedException("HasComponent does not support " + typeof(T).FullName);
        }

        [DllImport("__Internal")]
        internal static extern InteropBool Native_MeshComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_LightComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_ScriptComponent_Exists(uint entityHandle, IntPtr sceneHandle);
    }
}
