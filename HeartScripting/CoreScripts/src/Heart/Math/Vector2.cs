using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 8)]
    internal struct Vector2Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
    }

    public class Vector2
    {
        private float _x, _y = 0.0F;

        public Vector2(float x, float y, float z)
        {
            _x = x;
            _y = y;
        }

        public Vector2(Vector2 other)
        {
            _x = other._x;
            _y = other._y;
        }

        internal Vector2(Vector2Internal other)
        {
            _x = other.X;
            _y = other.Y;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal Vector2Internal ToVector2Internal()
        {
            return new Vector2Internal
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
    }
}
