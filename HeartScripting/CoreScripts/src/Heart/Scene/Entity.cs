using Heart.Container;
using Heart.Core;
using Heart.Math;
using Heart.NativeBridge;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public partial class Entity
    {
        internal Entity()
        { }

        internal Entity(uint entityHandle, IntPtr sceneHandle)
        {
            _entityHandle = entityHandle;
            _sceneHandle = sceneHandle;
        }

        public bool IsAlive()
        {
            if (_entityHandle == InvalidEntityHandle) return false;
            return NativeMarshal.InteropBoolToBool(
                Native_Entity_IsValid(_entityHandle, _sceneHandle)
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Scene GetScene()
            => new Scene(_sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID GetId()
            => ComponentUtils.GetId(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public string GetName()
            => ComponentUtils.GetName(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetName(string name)
            => ComponentUtils.SetName(_entityHandle, _sceneHandle, name);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID GetParent()
            => ComponentUtils.GetParent(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetParent(UUID uuid)
            => ComponentUtils.SetParent(_entityHandle, _sceneHandle, uuid);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID GetChildId(uint index)
            => ComponentUtils.GetChildId(_entityHandle, _sceneHandle, index);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity GetChild(uint index)
            => ComponentUtils.GetChild(_entityHandle, _sceneHandle, index);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public UUID[] GetChildrenIds()
            => ComponentUtils.GetChildrenIds(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity[] GetChildren()
            => ComponentUtils.GetChildren(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public uint GetChildrenCount()
            => ComponentUtils.GetChildrenCount(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void AddChild(UUID uuid)
            => ComponentUtils.AddChild(_entityHandle, _sceneHandle, uuid);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void AddChild(Entity entity)
            => ComponentUtils.AddChild(_entityHandle, _sceneHandle, entity.GetId());

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Entity CreateChild(string name)
            => ComponentUtils.CreateChild(_entityHandle, _sceneHandle, name);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void RemoveChild(UUID uuid)
            => ComponentUtils.RemoveChild(_entityHandle, _sceneHandle, uuid);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void RemoveChild(Entity entity)
            => ComponentUtils.RemoveChild(_entityHandle, _sceneHandle, entity.GetId());

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetPosition()
            => ComponentUtils.GetPosition(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetRotation()
            => ComponentUtils.GetRotation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetScale()
            => ComponentUtils.GetScale(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetPosition(Vec3 position)
            => ComponentUtils.SetPosition(_entityHandle, _sceneHandle, position);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRotation(Vec3 rotation)
            => ComponentUtils.SetRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(Vec3 scale)
            => ComponentUtils.SetScale(_entityHandle, _sceneHandle, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetTransform(Vec3 position, Vec3 rotation, Vec3 scale)
            => ComponentUtils.SetTransform(_entityHandle, _sceneHandle, position, rotation, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void ApplyRotation(Vec3 rotation)
            => ComponentUtils.ApplyRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetForwardVector()
            => ComponentUtils.GetForwardVector(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T GetComponent<T>() where T : class, IComponent, new()
            => ComponentUtils.GetComponent<T>(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool HasComponent<T>() where T : class, IComponent, new()
            => ComponentUtils.HasComponent<T>(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T AddComponent<T>() where T : class, IComponent, new()
            => ComponentUtils.AddComponent<T>(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void RemoveComponent<T>() where T : class, IComponent, new()
            => ComponentUtils.RemoveComponent<T>(_entityHandle, _sceneHandle);

        public void Destroy()
        {
            Native_Entity_Destroy(_entityHandle, _sceneHandle);
            _entityHandle = InvalidEntityHandle;
        }

        internal static uint InvalidEntityHandle = uint.MaxValue;

        internal uint _entityHandle = InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        [UnmanagedCallback]
        internal static partial void Native_Entity_Destroy(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial InteropBool Native_Entity_IsValid(uint entityHandle, IntPtr sceneHandle);
    }
}
