using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Math
{
    [StructLayout(LayoutKind.Explicit, Size = 64)]
    internal struct Mat4x4Internal
    {
        [FieldOffset(0)] public float C1R1;
        [FieldOffset(4)] public float C1R2;
        [FieldOffset(8)] public float C1R3;
        [FieldOffset(12)] public float C1R4;
        [FieldOffset(16)] public float C2R1;
        [FieldOffset(20)] public float C2R2;
        [FieldOffset(24)] public float C2R3;
        [FieldOffset(28)] public float C2R4;
        [FieldOffset(32)] public float C3R1;
        [FieldOffset(36)] public float C3R2;
        [FieldOffset(40)] public float C3R3;
        [FieldOffset(44)] public float C3R4;
        [FieldOffset(48)] public float C4R1;
        [FieldOffset(52)] public float C4R2;
        [FieldOffset(56)] public float C4R3;
        [FieldOffset(60)] public float C4R4;
    }

    public class Mat4x4
    {
        internal Mat4x4Internal _internal;

        public Mat4x4()
        {
            _internal.C1R1 = 0.0f;
            _internal.C1R2 = 0.0f;
            _internal.C1R3 = 0.0f;
            _internal.C1R4 = 0.0f;
            _internal.C2R1 = 0.0f;
            _internal.C2R2 = 0.0f;
            _internal.C2R3 = 0.0f;
            _internal.C2R4 = 0.0f;
            _internal.C3R1 = 0.0f;
            _internal.C3R2 = 0.0f;
            _internal.C3R3 = 0.0f;
            _internal.C3R4 = 0.0f;
            _internal.C4R1 = 0.0f;
            _internal.C4R2 = 0.0f;
            _internal.C4R3 = 0.0f;
            _internal.C4R4 = 0.0f;
        }

        public Mat4x4(
            float c1r1, float c1r2, float c1r3, float c1r4,
            float c2r1, float c2r2, float c2r3, float c2r4,
            float c3r1, float c3r2, float c3r3, float c3r4,
            float c4r1, float c4r2, float c4r3, float c4r4
        )
        {
            _internal.C1R1 = c1r1;
            _internal.C1R2 = c1r2;
            _internal.C1R3 = c1r3;
            _internal.C1R4 = c1r4;
            _internal.C2R1 = c2r1;
            _internal.C2R2 = c2r2;
            _internal.C2R3 = c2r3;
            _internal.C2R4 = c2r4;
            _internal.C3R1 = c3r1;
            _internal.C3R2 = c3r2;
            _internal.C3R3 = c3r3;
            _internal.C3R4 = c3r4;
            _internal.C4R1 = c4r1;
            _internal.C4R2 = c4r2;
            _internal.C4R3 = c4r3;
            _internal.C4R4 = c4r4;
        }

        public Mat4x4(Mat4x4 other)
        {
            _internal = other._internal;
        }

        internal Mat4x4(Mat4x4Internal other)
        {
            _internal = other;
        }

        public float C1R1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C1R1;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C1R1 = value;
        }

        public float C1R2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C1R2;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C1R2 = value;
        }

        public float C1R3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C1R3;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C1R3 = value;
        }

        public float C1R4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C1R4;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C1R4 = value;
        }

        public float C2R1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C2R1;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C2R1 = value;
        }

        public float C2R2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C2R2;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C2R2 = value;
        }

        public float C2R3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C2R3;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C2R3 = value;
        }

        public float C2R4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C2R4;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C2R4 = value;
        }

        public float C3R1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C3R1;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C3R1 = value;
        }

        public float C3R2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C3R2;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C3R2 = value;
        }

        public float C3R3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C3R3;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C3R3 = value;
        }

        public float C3R4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C3R4;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C3R4 = value;
        }

        public float C4R1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C4R1;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C4R1 = value;
        }

        public float C4R2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C4R2;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C4R2 = value;
        }

        public float C4R3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C4R3;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C4R3 = value;
        }

        public float C4R4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internal.C4R4;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _internal.C4R4 = value;
        }

        public Vec4 C1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C1R1, _internal.C1R2, _internal.C1R3, _internal.C1R4);
            set
            {
               _internal.C1R1 = value.X;
               _internal.C1R2 = value.Y;
               _internal.C1R3 = value.Z;
               _internal.C1R4 = value.W;
            }
        }

        public Vec4 C2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C2R1, _internal.C2R2, _internal.C2R3, _internal.C2R4);
            set
            {
               _internal.C2R1 = value.X;
               _internal.C2R2 = value.Y;
               _internal.C2R3 = value.Z;
               _internal.C2R4 = value.W;
            }
        }

        public Vec4 C3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C3R1, _internal.C3R2, _internal.C3R3, _internal.C3R4);
            set
            {
               _internal.C3R1 = value.X;
               _internal.C3R2 = value.Y;
               _internal.C3R3 = value.Z;
               _internal.C3R4 = value.W;
            }
        }

        public Vec4 C4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C4R1, _internal.C4R2, _internal.C4R3, _internal.C4R4);
            set
            {
               _internal.C4R1 = value.X;
               _internal.C4R2 = value.Y;
               _internal.C4R3 = value.Z;
               _internal.C4R4 = value.W;
            }
        }

        public Vec4 R1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C1R1, _internal.C2R1, _internal.C3R1, _internal.C4R1);
            set
            {
               _internal.C1R1 = value.X;
               _internal.C2R1 = value.Y;
               _internal.C3R1 = value.Z;
               _internal.C4R1 = value.W;
            }
        }

        public Vec4 R2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C1R2, _internal.C2R2, _internal.C3R2, _internal.C4R2);
            set
            {
               _internal.C1R2 = value.X;
               _internal.C2R2 = value.Y;
               _internal.C3R2 = value.Z;
               _internal.C4R2 = value.W;
            }
        }

        public Vec4 R3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C1R3, _internal.C2R3, _internal.C3R3, _internal.C4R3);
            set
            {
               _internal.C1R3 = value.X;
               _internal.C2R3 = value.Y;
               _internal.C3R3 = value.Z;
               _internal.C4R3 = value.W;
            }
        }

        public Vec4 R4
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new Vec4(_internal.C1R4, _internal.C2R4, _internal.C3R4, _internal.C4R4);
            set
            {
               _internal.C1R4 = value.X;
               _internal.C2R4 = value.Y;
               _internal.C3R4 = value.Z;
               _internal.C4R4 = value.W;
            }
        }
    }
}
