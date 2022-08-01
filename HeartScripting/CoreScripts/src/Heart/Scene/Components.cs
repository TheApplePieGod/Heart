using Heart.Math;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 36)]
    internal struct TransformComponentInternal
    {
        [FieldOffset(0)] public Vector3Internal Translation;
        [FieldOffset(12)] public Vector3Internal Rotation;
        [FieldOffset(24)] public Vector3Internal Scale;
    }

    public interface IComponent { }

    public class TransformComponent : IComponent
    {
        internal unsafe TransformComponentInternal* _internalValue;
        internal uint _entityHandle;
        internal IntPtr _sceneHandle;

        internal unsafe TransformComponent(uint entityHandle, IntPtr sceneHandle)
        {
            _entityHandle = entityHandle;
            _sceneHandle = sceneHandle;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private unsafe void RefreshPtr()
        {
            // We shouldn't need safety checking here because all entities are guaranteed
            // to have a transform comp
            Native_TransformComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            //if (_internalValue == null)
            //    throw new InvalidOperationException("Attempting to read or modify transform component that no longer exists");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void CacheTransform()
            => Native_TransformComponent_CacheTransform(_entityHandle, _sceneHandle);

        public unsafe Vector3 Translation
        {
            get
            {
                RefreshPtr();
                return new Vector3(_internalValue->Translation);
            }
            set
            {
                RefreshPtr();
                _internalValue->Translation = value.ToInternal();
                CacheTransform();
            }
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TransformComponent_Get(uint entityHandle, IntPtr sceneHandle, out TransformComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_TransformComponent_CacheTransform(uint entityHandle, IntPtr sceneHandle);
    }
}
