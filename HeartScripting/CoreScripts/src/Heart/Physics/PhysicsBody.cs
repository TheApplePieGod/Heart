using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Physics
{
    public enum PhysicsBodyType : uint
    {
        None = 0,
        Rigid, Ghost
    }

    public enum PhysicsBodyShape : uint
    {
        None = 0,
        Box, Sphere, Capsule
    }

    [StructLayout(LayoutKind.Explicit, Size = 20)]
    internal struct PhysicsBodyInfoInternal
    {
        [FieldOffset(0)] public uint Type;
        [FieldOffset(4)] public float Mass;
        [FieldOffset(8)] public uint CollisionChannels;
        [FieldOffset(12)] public uint CollisionMask;
        [FieldOffset(16)] public IntPtr ExtraData;
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
            _internal.Type = (uint)PhysicsBodyType.Rigid;
            _internal.Mass = 1.0f;
            _internal.CollisionChannels = (uint)DefaultCollisionChannel.Default;
            _internal.CollisionMask = (uint)DefaultCollisionChannel.All;
        }

        public PhysicsBodyInfo(PhysicsBodyType type, float mass, uint colChannels, uint colMask) 
        {
            _internal.Type = (uint)type;
            _internal.Mass = mass;
            _internal.CollisionChannels = colChannels;
            _internal.CollisionMask = colMask;
        }

        public PhysicsBodyInfo(PhysicsBodyInfo other)
        {
            _internal = other._internal;
        }

        internal PhysicsBodyInfo(PhysicsBodyInfoInternal other)
        {
            _internal = other;
        }

        public PhysicsBodyType Type
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => (PhysicsBodyType)_internal.Type;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.Type = (uint)value;
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