using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Math;
using Heart.NativeInterop;
using Heart.Physics;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 4)]
    internal struct CollisionComponentInternal
    {
        [FieldOffset(0)] public uint BodyId;
    }

    public class CollisionComponent : Component
    {
        public CollisionComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal CollisionComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

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

        [DllImport("__Internal")]
        internal static extern unsafe void Native_CollisionComponent_Get(uint entityHandle, IntPtr sceneHandle, out CollisionComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_CollisionComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_GetInfo(uint entityHandle, IntPtr sceneHandle, out PhysicsBodyInfoInternal info);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_GetShapeType(uint entityHandle, IntPtr sceneHandle, out uint type);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UpdateType(uint entityHandle, IntPtr sceneHandle, uint type);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UpdateMass(uint entityHandle, IntPtr sceneHandle, float mass);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UpdateCollisionChannels(uint entityHandle, IntPtr sceneHandle, ulong channels);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UpdateCollisionMask(uint entityHandle, IntPtr sceneHandle, ulong mask);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UseBoxShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, Vec3Internal extent);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UseSphereShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius);

        [DllImport("__Internal")]
        internal static extern void Native_CollisionComponent_UseCapsuleShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius, float halfHeight);
    }
}
