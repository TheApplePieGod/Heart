using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.Math;
using Heart.NativeInterop;

namespace Heart.Scene
{
    [StructLayout(LayoutKind.Explicit, Size = 4)]
    internal struct RigidBodyComponentInternal
    {
        [FieldOffset(0)] public uint BodyId;
    }

    public class RigidBodyComponent : Component
    {
        public RigidBodyComponent()
            : base(Entity.InvalidEntityHandle, IntPtr.Zero)
        { }

        internal RigidBodyComponent(uint entityHandle, IntPtr sceneHandle)
            : base(entityHandle, sceneHandle)
        {}

        public PhysicsBodyInfo GetInfo()
        {
            Native_RigidBodyComponent_GetInfo(_entityHandle, _sceneHandle, out var info);
            return new PhysicsBodyInfo(info);
        }

        public PhysicsBodyType GetBodyType()
        {
            Native_RigidBodyComponent_GetType(_entityHandle, _sceneHandle, out var type);            
            return (PhysicsBodyType)type;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateMass(float mass)
            => Native_RigidBodyComponent_UpdateMass(_entityHandle, _sceneHandle, mass);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateCollisionChannels(ulong channels)
            => Native_RigidBodyComponent_UpdateCollisionChannels(_entityHandle, _sceneHandle, channels);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UpdateCollisionMask(ulong mask)
            => Native_RigidBodyComponent_UpdateCollisionMask(_entityHandle, _sceneHandle, mask);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseBoxShape(PhysicsBodyInfo info, Vec3 halfExtent)
            => Native_RigidBodyComponent_UseBoxShape(_entityHandle, _sceneHandle, info.ToInternal(), halfExtent.ToVec3Internal());

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseSphereShape(PhysicsBodyInfo info, float radius)
            => Native_RigidBodyComponent_UseSphereShape(_entityHandle, _sceneHandle, info.ToInternal(), radius);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void UseCapsuleShape(PhysicsBodyInfo info, float radius, float halfHeight)
            => Native_RigidBodyComponent_UseCapsuleShape(_entityHandle, _sceneHandle, info.ToInternal(), radius, halfHeight);

        [DllImport("__Internal")]
        internal static extern unsafe void Native_RigidBodyComponent_Get(uint entityHandle, IntPtr sceneHandle, out RigidBodyComponentInternal* comp);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_Add(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_Remove(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_RigidBodyComponent_Exists(uint entityHandle, IntPtr sceneHandle);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_GetInfo(uint entityHandle, IntPtr sceneHandle, out PhysicsBodyInfoInternal info);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_GetType(uint entityHandle, IntPtr sceneHandle, out uint type);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UpdateMass(uint entityHandle, IntPtr sceneHandle, float mass);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UpdateCollisionChannels(uint entityHandle, IntPtr sceneHandle, ulong channels);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UpdateCollisionMask(uint entityHandle, IntPtr sceneHandle, ulong mask);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UseBoxShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, Vec3Internal extent);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UseSphereShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius);

        [DllImport("__Internal")]
        internal static extern void Native_RigidBodyComponent_UseCapsuleShape(uint entityHandle, IntPtr sceneHandle, in PhysicsBodyInfoInternal info, float radius, float halfHeight);
    }
}
