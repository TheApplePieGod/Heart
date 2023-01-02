using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal struct Vec4Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
        [FieldOffset(8)] public float Z;
        [FieldOffset(12)] public float W;
    }

    public class Vec4
    {
        internal Vec4Internal _internal;

        public Vec4()
        {
            _internal.X = 0.0f;
            _internal.Y = 0.0f;
            _internal.Z = 0.0f;
            _internal.W = 0.0f;
        }

        public Vec4(float x, float y, float z, float w)
        {
            _internal.X = x;
            _internal.Y = y;
            _internal.Z = z;
            _internal.W = w;
        }

        public Vec4(Vec4 other)
        {
            _internal = other._internal;
        }

        internal Vec4(Vec4Internal other)
        {
            _internal = other;
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

        public float W
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.W;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.W = value;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator -(Vec4 a)
            => new Vec4(-a.X, -a.Y, -a.Z, -a.W);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator +(Vec4 a, Vec4 b)
            => new Vec4(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator -(Vec4 a, Vec4 b)
            => new Vec4(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator *(Vec4 a, Vec4 b)
            => new Vec4(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator /(Vec4 a, Vec4 b)
            => new Vec4(a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator +(Vec4 a, float b)
            => new Vec4(a.X + b, a.Y + b, a.Z + b, a.W + b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator -(Vec4 a, float b)
            => new Vec4(a.X - b, a.Y - b, a.Z - b, a.W - b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator *(Vec4 a, float b)
            => new Vec4(a.X * b, a.Y * b, a.Z * b, a.W * b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec4 operator /(Vec4 a, float b)
            => new Vec4(a.X / b, a.Y / b, a.Z / b, a.W / b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object other)
            => Equals(other as Vec4);

        public bool Equals(Vec4 other)
        {
            if (other == null) return false;
            if (object.ReferenceEquals(this, other)) return true;
            return (
                _internal.X == other.X &&
                _internal.Y == other.Y &&
                _internal.Z == other.Z &&
                _internal.W == other.W
            );
        }

        public override int GetHashCode()
            => base.GetHashCode();

        public static bool operator ==(Vec4 a, Vec4 b)
        {
            if (object.ReferenceEquals(a, b)) return true;
            if ((object)a == null || (object)b == null) return false;
            return (
                a.X == b.X &&
                a.Y == b.Y &&
                a.Z == b.Z &&
                a.W == b.W
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vec4 a, Vec4 b)
            => !(a == b);
    }
}
