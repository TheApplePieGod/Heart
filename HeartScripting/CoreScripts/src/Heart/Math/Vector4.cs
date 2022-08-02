using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal struct Vector4Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
        [FieldOffset(8)] public float Z;
        [FieldOffset(12)] public float W;
    }

    public class Vector4
    {
        private float _x, _y, _z, _w = 0.0F;

        public Vector4(float x, float y, float z, float w)
        {
            _x = x;
            _y = y;
            _z = z;
            _w = w;
        }

        public Vector4(Vector4 other)
        {
            _x = other._x;
            _y = other._y;
            _z = other._z;
            _w = other._w;
        }

        internal Vector4(Vector4Internal other)
        {
            _x = other.X;
            _y = other.Y;
            _z = other.Z;
            _w = other.W;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal Vector4Internal ToVector4Internal()
        {
            return new Vector4Internal
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
