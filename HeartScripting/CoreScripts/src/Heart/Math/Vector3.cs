using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 12)]
    internal struct Vector3Internal
    {
        [FieldOffset(0)] public float X;
        [FieldOffset(4)] public float Y;
        [FieldOffset(8)] public float Z;
    }

    public class Vector3
    {
        private float _x, _y, _z = 0.0F;

        public Vector3(float x, float y, float z)
        {
            _x = x;
            _y = y;
            _z = z;
        }

        public Vector3(Vector3 other)
        {
            _x = other._x;
            _y = other._y;
            _z = other._z;
        }

        internal Vector3(Vector3Internal other)
        {
            _x = other.X;
            _y = other.Y;
            _z = other.Z;
        }

        internal Vector3Internal ToInternal()
        {
            return new Vector3Internal
            {
                X = _x,
                Y = _y,
                Z = _z
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
    }
}
