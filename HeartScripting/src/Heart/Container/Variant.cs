using Heart.NativeInterop;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    public enum VariantType : uint
    {
        None = 0,
        Bool, Int, Float,
        String, Array
    }

    // Variants do not clean up by themselves. Either Free() must be manually called
    // or the native destructor will automatically run if stored in an HArray and
    // cleaned up
    [StructLayout(LayoutKind.Explicit)]
    internal ref struct Variant
    {
        [StructLayout(LayoutKind.Explicit, Size=8)]
        private unsafe ref struct Data
        {
            [FieldOffset(0)] public InteropBool Bool;
            [FieldOffset(0)] public int Int;
            [FieldOffset(0)] public float Float;

            // 'Any' mem fields
            [FieldOffset(0)] public HArrayInternal Array;
        }

        // For some reason VariantType takes up 8 bytes, so we also
        // alignas() in the native variant struct
        [FieldOffset(0)] private VariantType _type;
        [FieldOffset(8)] private Data _data;
        
        public void Destroy()
        {
            Native_Variant_Destroy(ref this);
        }

        public VariantType Type
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _type;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => _type = value;
        }

        public InteropBool Bool
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _data.Bool;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set { _data.Bool = value; _type = VariantType.Bool; }
        }

        public int Int
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _data.Int;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set { _data.Int = value; _type = VariantType.Int; }
        }

        public float Float
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _data.Float;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set { _data.Float = value; _type = VariantType.Float; }
        }

        public HArrayInternal Array
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _data.Array;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set { _data.Array = value; _type = VariantType.Array; }
        }

        [DllImport("__Internal")]
        private static extern void Native_Variant_Destroy([In] ref Variant variant);
    }
}
