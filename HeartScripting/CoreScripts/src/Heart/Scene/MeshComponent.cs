using Heart.Container;
using Heart.NativeInterop;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal unsafe struct MeshComponentInternal
    {
        [FieldOffset(0)] public UUID Mesh;
        [FieldOffset(8)] public UUID* Materials;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public ContainerInfo* GetMaterialInfo()
            => (ContainerInfo*)Materials - 1;
    }

    public partial class MeshComponent : IComponent<MeshComponent>
    {
        internal unsafe MeshComponentInternal* _internalValue;
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        private unsafe void RefreshPtr()
        {
            Native_MeshComponent_Get(_entityHandle, _sceneHandle, out _internalValue);
            if (_internalValue == null)
                throw new InvalidOperationException("Attempting to read or modify mesh component that no longer exists");
        }

        public unsafe UUID Mesh
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

        public unsafe uint MaterialCount
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                RefreshPtr();
                return _internalValue->GetMaterialInfo()->ElemCount;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe UUID GetMaterial(uint index)
        {
            RefreshPtr();
            return _internalValue->Materials[index];
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe UUID[] GetMaterials(uint index)
        {
            RefreshPtr();
            return NativeMarshal.PtrToArray(_internalValue->Materials, _internalValue->GetMaterialInfo()->ElemCount);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void SetMaterial(uint index, UUID material)
        {
            RefreshPtr();
            _internalValue->Materials[index] = material;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void SetMaterials(UUID[] materials)
        {
            RefreshPtr();
            NativeMarshal.CopyArrayToPtr(materials, _internalValue->Materials, _internalValue->GetMaterialInfo()->ElemCount);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe void AddMaterial(UUID material)
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
            Native_MeshComponent_RemoveMaterial(_entityHandle, _sceneHandle, MaterialCount - 1);
        }

        public static unsafe InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_MeshComponent_Exists(entityHandle, sceneHandle);
        public static unsafe void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_MeshComponent_Add(entityHandle, sceneHandle);
        public static unsafe void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_MeshComponent_Remove(entityHandle, sceneHandle);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_MeshComponent_Get(uint entityHandle, IntPtr sceneHandle, out MeshComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_MeshComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_AddMaterial(uint entityHandle, IntPtr sceneHandle, UUID material);

        [DllImport("__Internal")]
        internal static extern void Native_MeshComponent_RemoveMaterial(uint entityHandle, IntPtr sceneHandle, uint index);
    }
}
