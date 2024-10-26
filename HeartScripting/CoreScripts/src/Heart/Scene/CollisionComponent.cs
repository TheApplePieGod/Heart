using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Math;
using Heart.NativeInterop;
using Heart.NativeBridge;
using Heart.Physics;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 4)]
    internal struct CollisionComponentInternal
    {
        [FieldOffset(0)] public uint BodyId;
    }

    public partial class CollisionComponent : IComponent
    {
        internal uint _entityHandle = Entity.InvalidEntityHandle;
        internal IntPtr _sceneHandle = IntPtr.Zero;

        public PhysicsBodyInfo GetInfo()
        {
            Native_CollisionComponent_GetInfo(_entityHandle, _sceneHandle, out var info);
            return new PhysicsBodyInfo(info);
        }

        public PhysicsBodyShape GetShapeType()
        {
            Native_CollisionComponent_GetShapeType(_entityHandle, _sceneHandle, out var type);
            return (PhysicsBodyShape)type;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateType(PhysicsBodyType type)
            => Native_CollisionComponent_UpdateType(_entityHandle, _sceneHandle, (uint)type);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateMass(float mass)
            => Native_CollisionComponent_UpdateMass(_entityHandle, _sceneHandle, mass);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateCollisionChannels(ulong channels)
            => Native_CollisionComponent_UpdateCollisionChannels(_entityHandle, _sceneHandle, channels);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateCollisionMask(ulong mask)
            => Native_CollisionComponent_UpdateCollisionMask(_entityHandle, _sceneHandle, mask);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseBoxShape(PhysicsBodyInfo info, Vec3 halfExtent)
            => Native_CollisionComponent_UseBoxShape(_entityHandle, _sceneHandle, info._internal, halfExtent._internal);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseSphereShape(PhysicsBodyInfo info, float radius)
            => Native_CollisionComponent_UseSphereShape(_entityHandle, _sceneHandle, info._internal, radius);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseCapsuleShape(PhysicsBodyInfo info, float radius, float halfHeight)
            => Native_CollisionComponent_UseCapsuleShape(_entityHandle, _sceneHandle, info._internal, radius, halfHeight);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static InteropBool NativeExists(uint entityHandle, IntPtr sceneHandle)
            => Native_CollisionComponent_Exists(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeAdd(uint entityHandle, IntPtr sceneHandle)
            => Native_CollisionComponent_Add(entityHandle, sceneHandle);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void NativeRemove(uint entityHandle, IntPtr sceneHandle)
            => Native_CollisionComponent_Remove(entityHandle, sceneHandle);

        [UnmanagedCallback]
        internal static unsafe partial void Native_CollisionComponent_Get(uint entityHandle, IntPtr sceneHandle, out CollisionComponentInternal* comp);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial InteropBool Native_CollisionComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_GetInfo(uint entityHandle, IntPtr sceneHandle, out PhysicsBodyInfoInternal info);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_GetShapeType(uint entityHandle, IntPtr sceneHandle, out uint type);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UpdateType(uint entityHandle, IntPtr sceneHandle, uint type);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UpdateMass(uint entityHandle, IntPtr sceneHandle, float mass);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UpdateCollisionChannels(uint entityHandle, IntPtr sceneHandle, ulong channels);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UpdateCollisionMask(uint entityHandle, IntPtr sceneHandle, ulong mask);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UseBoxShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, Vec3Internal extent);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UseSphereShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius);

        [UnmanagedCallback]
        internal static partial void Native_CollisionComponent_UseCapsuleShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius, float halfHeight);
    }
}
