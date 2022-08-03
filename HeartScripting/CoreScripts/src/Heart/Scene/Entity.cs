using Heart.Container;
using Heart.Core;
using Heart.Math;
using Heart.NativeBridge;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public class Entity
    {
        // Client callable methods
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
        public Vec3 GetTranslation()
            => ComponentUtils.GetTranslation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetRotation()
            => ComponentUtils.GetRotation(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vec3 GetScale()
            => ComponentUtils.GetScale(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetTranslation(Vec3 translation)
            => ComponentUtils.SetTranslation(_entityHandle, _sceneHandle, translation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRotation(Vec3 rotation)
            => ComponentUtils.SetRotation(_entityHandle, _sceneHandle, rotation);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetScale(Vec3 scale)
            => ComponentUtils.SetScale(_entityHandle, _sceneHandle, scale);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T GetComponent<T>() where T : Component
            => ComponentUtils.GetComponent<T>(_entityHandle, _sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool HasComponent<T>() where T : Component
            => ComponentUtils.HasComponent<T>(_entityHandle, _sceneHandle);

        // Internal fields
        internal uint _entityHandle = uint.MaxValue;
        internal IntPtr _sceneHandle = IntPtr.Zero;
    }
}
