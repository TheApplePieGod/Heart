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
        private float _x, _y, _z, _w = 0.0F;

        public Vec4(float x, float y, float z, float w)
        {
            _x = x;
            _y = y;
            _z = z;
            _w = w;
        }

        public Vec4(Vec4 other)
        {
            _x = other._x;
            _y = other._y;
            _z = other._z;
            _w = other._w;
        }

        internal Vec4(Vec4Internal other)
        {
            _x = other.X;
            _y = other.Y;
            _z = other.Z;
            _w = other.W;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal Vec4Internal ToVec4Internal()
        {
            return new Vec4Internal
            {
                X = _x,
                Y = _y,
                Z = _z,
                W = _w,
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

        public float Z
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _z;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _z = value;
        }

        public float W
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _w;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _w = value;
        }
    }
}
