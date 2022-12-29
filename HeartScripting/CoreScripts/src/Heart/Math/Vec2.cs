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
    }
}
