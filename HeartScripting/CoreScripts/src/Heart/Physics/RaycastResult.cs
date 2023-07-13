using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Heart.NativeInterop;
using Heart.Math;
using Heart.Scene;

namespace Heart.Physics
{
    [StructLayout(LayoutKind.Explicit, Size = 36)]
    internal struct RaycastResultInternal
    {
        [FieldOffset(0)] public Vec3Internal HitLocation;
        [FieldOffset(12)] public Vec3Internal HitNormal;
        [FieldOffset(24)] public float HitFraction;
        [FieldOffset(28)] public UUID HitEntityId;
    }
    
    public class RaycastResult
    {
        internal RaycastResultInternal _internal;

        public RaycastResult()
        {
            _internal.HitLocation = new Vec3Internal { X = 0.0f, Y = 0.0f, Z = 0.0f };
            _internal.HitNormal = new Vec3Internal { X = 0.0f, Y = 0.0f, Z = 0.0f };
            _internal.HitFraction = 0.0f;
            _internal.HitEntityId = 0;
        }

        public RaycastResult(Vec3 hitLocation, Vec3 hitNormal, float hitFraction, UUID hitEntityId) 
        {
            _internal.HitLocation = hitLocation._internal;
            _internal.HitNormal = hitNormal._internal;
            _internal.HitFraction = hitFraction;
            _internal.HitEntityId = hitEntityId;
        }

        public RaycastResult(RaycastResult other)
        {
            _internal = other._internal;
        }

        internal RaycastResult(RaycastResultInternal other)
        {
            _internal = other;
        }
        
        public Vec3 HitLocation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec3(_internal.HitLocation);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.HitLocation = value._internal;
        }

        public Vec3 HitNormal
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec3(_internal.HitNormal);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.HitNormal = value._internal;
        }

        public float HitFraction
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.HitFraction;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.HitFraction = value;
        }

        public UUID HitEntityId
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.HitEntityId;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.HitEntityId = value;
        }
    }
}