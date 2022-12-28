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
        private float _x, _y = 0.0F;

        public Vec2() {}

        public Vec2(float x, float y)
        {
            _x = x;
            _y = y;
        }

        public Vec2(Vec2 other)
        {
            _x = other._x;
            _y = other._y;
        }

        internal Vec2(Vec2Internal other)
        {
            _x = other.X;
            _y = other.Y;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal Vec2Internal ToVec2Internal()
        {
            return new Vec2Internal
            {
                X = _x,
                Y = _y,
            };
        }

        public float X
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _x;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _x = value;
        }

        public float Y
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _y;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _y = value;
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
