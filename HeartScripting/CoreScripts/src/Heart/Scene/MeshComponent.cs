using Heart.Container;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal unsafe struct MeshComponentInternal
    {
        [FieldOffset(0)] public ulong Mesh;
        [FieldOffset(8)] public ulong* Materials;

        public ContainerInfo* GetInfo()
        {
            return (ContainerInfo*)Materials - 1;
        }
    }

    public class MeshComponent : Component
    {
        internal unsafe MeshComponentInternal* _internalValue;

        internal MeshComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        { }

        private unsafe void RefreshPtr()
        {
            Native_MeshComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify transform component that no longer exists");
        }

        public unsafe ulong Mesh
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->Mesh;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                RefreshPtr();
                _internalValue->Mesh = value;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe uint MaterialCount()
        {
            RefreshPtr();
            return _internalValue->GetInfo()->ElemCount;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ulong GetMaterial(uint index)
        {
            RefreshPtr();
            return _internalValue->Materials[index];
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ulong SetMaterial(uint index, ulong material)
        {
            RefreshPtr();
            return _internalValue->Materials[index] = material;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void AddMaterial(ulong material)
        {
            Native_MeshComponent_AddMaterial(_entityHandle, _sceneHandle, material);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void RemoveMaterial(uint index)
        {
            Native_MeshComponent_RemoveMaterial(_entityHandle, _sceneHandle, index);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void PopMaterial()
        {
            Native_MeshComponent_RemoveMaterial(_entityHandle, _sceneHandle, MaterialCount() - 1);
        }

        [DllImport("__Internal")]
        internal static extern unsafe void Native_MeshComponent_Get(uint entityHandle, IntPtr sceneHandle, out MeshComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_AddMaterial(uint entityHandle, IntPtr sceneHandle, ulong material);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_RemoveMaterial(uint entityHandle, IntPtr sceneHandle, uint index);
    }
}
