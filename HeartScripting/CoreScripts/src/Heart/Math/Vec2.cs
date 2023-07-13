using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 8)]
    internal struct Vec2Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
    }

    public class Vec2
    {
        internal Vec2Internal _internal;

        public Vec2()
        {
            _internal.X = 0.0f;
            _internal.Y = 0.0f;
        }

        public Vec2(float x, float y)
        {
            _internal.X = x;
            _internal.Y = y;
        }

        public Vec2(Vec2 other)
        {
            _internal = other._internal;
        }

        internal Vec2(Vec2Internal other)
        {
            _internal = other;
        }

        public float Dot(Vec2 other)
            => _internal.X * other.X + _internal.Y * other.Y;

        public float GetMagnitude()
            => System.MathF.Sqrt(_internal.X * _internal.X + _internal.Y * _internal.Y);
        
        public Vec2 Normalize()
        {
            float mag = GetMagnitude();
            return new Vec2(_internal.X / mag, _internal.Y / mag);
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator -(Vec2 a)
            => new Vec2(-a.X, -a.Y);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator +(Vec2 a, Vec2 b)
            => new Vec2(a.X + b.X, a.Y + b.Y);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator -(Vec2 a, Vec2 b)
            => new Vec2(a.X - b.X, a.Y - b.Y);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator *(Vec2 a, Vec2 b)
            => new Vec2(a.X * b.X, a.Y * b.Y);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator /(Vec2 a, Vec2 b)
            => new Vec2(a.X / b.X, a.Y / b.Y);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator +(Vec2 a, float b)
            => new Vec2(a.X + b, a.Y + b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator -(Vec2 a, float b)
            => new Vec2(a.X - b, a.Y - b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator *(Vec2 a, float b)
            => new Vec2(a.X * b, a.Y * b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vec2 operator /(Vec2 a, float b)
            => new Vec2(a.X / b, a.Y / b);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object other)
            => Equals(other as Vec2);

        public bool Equals(Vec2 other)
        {
            if (other == null) return false;
            if (object.ReferenceEquals(this, other)) return true;
            return (
                _internal.X == other.X &&
                _internal.Y == other.Y
            );
        }

        public override int GetHashCode()
            => base.GetHashCode();

        public static bool operator ==(Vec2 a, Vec2 b)
        {
            if (object.ReferenceEquals(a, b)) return true;
            if ((object)a == null || (object)b == null) return false;
            return (
                a.X == b.X &&
                a.Y == b.Y
            );
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vec2 a, Vec2 b)
            => !(a == b);
    }
}
