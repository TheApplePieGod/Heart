using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 12)]
    internal struct Vec3Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
        [FieldOffset(8)] public float Z;
    }

    public class Vec3
    {
        internal Vec3Internal _internal;

        public Vec3()
        {
            _internal.X = 0.0f;
            _internal.Y = 0.0f;
            _internal.Z = 0.0f;
        }

        public Vec3(float x, float y, float z)
        {
            _internal.X = x;
            _internal.Y = y;
            _internal.Z = z;
        }

        public Vec3(Vec3 other)
        {
            _internal = other._internal;
        }

        internal Vec3(Vec3Internal other)
        {
            _internal = other;
        }

        internal Vec3(Vec4Internal other)
        {
            _internal.X = other.X;
            _internal.Y = other.Y;
            _internal.Z = other.Z;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal Vec4Internal ToVec4Internal(float w)
        {
            return new Vec4Internal
            {
                X = _internal.X,
                Y = _internal.Y,
                Z = _internal.Z,
                W = w
            };
        }
        
        public float Dot(Vec3 other)
            => _internal.X * other.X + _internal.Y * other.Y + _internal.Z * other.Z;
        
        public float GetMagnitude()
            => System.MathF.Sqrt(_internal.X * _internal.X + _internal.Y * _internal.Y + _internal.Z * _internal.Z);
        
        public Vec3 Normalize()
        {
            float mag = GetMagnitude();
            return new Vec3(_internal.X / mag, _internal.Y / mag, _internal.Z / mag);
        }

        public float X
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.X;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.X = value;
        }

        public float Y
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.Y;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.Y = value;
        }

        public float Z
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.Z;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.Z = value;
        }
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator -(Vec3 a)
            => new Vec3(-a.X, -a.Y, -a.Z);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator +(Vec3 a, Vec3 b)
            => new Vec3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator -(Vec3 a, Vec3 b)
            => new Vec3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator *(Vec3 a, Vec3 b)
            => new Vec3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator /(Vec3 a, Vec3 b)
            => new Vec3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator +(Vec3 a, float b)
            => new Vec3(a.X + b, a.Y + b, a.Z + b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator -(Vec3 a, float b)
            => new Vec3(a.X - b, a.Y - b, a.Z - b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator *(Vec3 a, float b)
            => new Vec3(a.X * b, a.Y * b, a.Z * b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec3 operator /(Vec3 a, float b)
            => new Vec3(a.X / b, a.Y / b, a.Z / b);
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object other)
            => Equals(other as Vec3);

        public bool Equals(Vec3 other)
        {
            if (other == null) return false;
            if (object.ReferenceEquals(this, other)) return true;
            return (
                _internal.X == other.X &&
                _internal.Y == other.Y &&
                _internal.Z == other.Z
            );
        }
        
        public override int GetHashCode()
            => base.GetHashCode();

        public static bool operator ==(Vec3 a, Vec3 b)
        {
            if (object.ReferenceEquals(a, b)) return true;
            if ((object)a == null || (object)b == null) return false;
            return (
                a.X == b.X &&
                a.Y == b.Y &&
                a.Z == b.Z
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vec3 a, Vec3 b)
            => !(a == b);
    }
}
