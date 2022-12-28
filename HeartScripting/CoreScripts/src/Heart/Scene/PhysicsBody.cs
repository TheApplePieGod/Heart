using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Scene
{
    public enum PhysicsBodyType : uint
    {
        None = 0,
        Box, Sphere, Capsule
    }

    [StructLayout(LayoutKind.Explicit, Size = 28)]
    internal struct PhysicsBodyInfoInternal
    {
        [FieldOffset(0)] public float Mass;
        [FieldOffset(4)] public ulong CollisionChannels;
        [FieldOffset(12)] public ulong CollisionMask;
        [FieldOffset(20)] public IntPtr ExtraData;
    }
    
    public enum DefaultCollisionChannel : ulong
    {
        All = unchecked((ulong)-1),
        Default = 1 << 0,
        Static = 1 << 1,
        Dynamic = 1 << 2
    }

    public class PhysicsBodyInfo
    {
        private float _mass = 1.0F;
        private ulong _collisionChannels = (ulong)DefaultCollisionChannel.Default;
        private ulong _collisionMask = (ulong)DefaultCollisionChannel.All;

        public PhysicsBodyInfo(float mass, ulong colChannels, ulong colMask) 
        {
            _mass = mass;
            _collisionChannels = colChannels;
            _collisionMask = colMask;
        }

        public PhysicsBodyInfo(PhysicsBodyInfo other)
        {
            _mass = other._mass;
            _collisionChannels = other._collisionChannels;
            _collisionMask = other._collisionMask;
        }

        internal PhysicsBodyInfo(PhysicsBodyInfoInternal other)
        {
            _mass = other.Mass;
            _collisionChannels = other.CollisionChannels;
            _collisionMask = other.CollisionMask;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal PhysicsBodyInfoInternal ToInternal()
        {
            return new PhysicsBodyInfoInternal
            {
                Mass = _mass,
                CollisionChannels = _collisionChannels,
                CollisionMask = _collisionMask,
                ExtraData = IntPtr.Zero
            };
        }

        public float Mass
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _mass;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _mass = value;
        }

        public ulong CollisionChannels
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _collisionChannels;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _collisionChannels = value;
        }

        public ulong CollisionMask
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _collisionMask;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _collisionMask = value;
        }
    }
}