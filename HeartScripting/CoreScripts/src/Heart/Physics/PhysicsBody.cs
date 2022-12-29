using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Physics
{
    public enum PhysicsBodyType : uint
    {
        None = 0,
        Box, Sphere, Capsule
    }

    [StructLayout(LayoutKind.Explicit, Size = 20)]
    internal struct PhysicsBodyInfoInternal
    {
        [FieldOffset(0)] public float Mass;
        [FieldOffset(4)] public uint CollisionChannels;
        [FieldOffset(8)] public uint CollisionMask;
        [FieldOffset(12)] public IntPtr ExtraData;
    }
    
    public enum DefaultCollisionChannel : uint
    {
        All = unchecked((uint)-1),
        Default = 1 << 0,
        Static = 1 << 1,
        Dynamic = 1 << 2
    }

    public class PhysicsBodyInfo
    {
        internal PhysicsBodyInfoInternal _internal;

        public PhysicsBodyInfo()
        {
            _internal.Mass = 1.0f;
            _internal.CollisionChannels = (uint)DefaultCollisionChannel.Default;
            _internal.CollisionMask = (uint)DefaultCollisionChannel.All;
        }

        public PhysicsBodyInfo(float mass, uint colChannels, uint colMask) 
        {
            _internal.Mass = mass;
            _internal.CollisionChannels = colChannels;
            _internal.CollisionMask = colMask;
        }

        public PhysicsBodyInfo(PhysicsBodyInfo other)
        {
            _internal.Mass = other.Mass;
            _internal.CollisionChannels = other.CollisionChannels;
            _internal.CollisionMask = other.CollisionMask;
        }

        internal PhysicsBodyInfo(PhysicsBodyInfoInternal other)
        {
            _internal.Mass = other.Mass;
            _internal.CollisionChannels = other.CollisionChannels;
            _internal.CollisionMask = other.CollisionMask;
        }

        public float Mass
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.Mass;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.Mass = value;
        }

        public uint CollisionChannels
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.CollisionChannels;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.CollisionChannels = value;
        }

        public uint CollisionMask
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.CollisionMask;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.CollisionMask = value;
        }
    }
}