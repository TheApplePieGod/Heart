using Heart.NativeInterop;
using System;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    internal static class VariantConverter
    {
        public static Variant ObjectToVariant(object obj)
        {
            switch (obj)
            {
                case null:
                     return new Variant();
                case bool value:
                    return BoolToVariant(value);
                case int value:
                    return IntToVariant(value);
                case float value:
                    return FloatToVariant(value);
                case HArray value:
                    return HArrayToVariant(value);
            }

            throw new NotImplementedException("C# Object -> Variant conversion not fully implemented");
        }

        public static object VariantToObject(Variant variant)
        {
            switch (variant.Type)
            {
                case VariantType.None:
                    return null;
                case VariantType.Bool:
                    return variant.Bool;
                case VariantType.Int:
                    return variant.Int;
                case VariantType.Float:
                    return variant.Float;
                case VariantType.Array:
                    return new HArray(variant.Array);
            }

            throw new NotImplementedException("C# Variant -> Object conversion not fully implemented");
        }

        public static Variant BoolToVariant(bool value)
            => new() { Type = VariantType.Bool, Bool = NativeMarshal.BoolToInteropBool(value) };
        public static Variant IntToVariant(int value)
            => new() { Type = VariantType.Int, Int = value };
        public static Variant FloatToVariant(float value)
            => new() { Type = VariantType.Float, Float = value };
        public static Variant HArrayToVariant(HArray value)
        {
            Variant v = new Variant();
            Native_Variant_FromHArray(ref v, ref value._internalVal);
            return v;
        }

        [DllImport("__Internal")]
        private static extern void Native_Variant_FromHArray([In, Out] ref Variant variant, [In] ref HArrayInternal value);
    }
}
