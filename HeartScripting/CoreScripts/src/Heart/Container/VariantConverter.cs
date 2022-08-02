using Heart.NativeInterop;
using System;
using System.Collections;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    public static class VariantConverter
    {
        public static Variant ObjectToVariant(object obj)
        {
            switch (obj)
            {
                case null:
                     return new Variant();
                case bool value:
                    return BoolToVariant(value);
                case sbyte value:
                    return IntToVariant(value);
                case short value:
                    return IntToVariant(value);
                case int value:
                    return IntToVariant(value);
                case long value:
                    return IntToVariant(value);
                case byte value:
                    return UIntToVariant(value);
                case ushort value:
                    return UIntToVariant(value);
                case uint value:
                    return UIntToVariant(value);
                case ulong value:
                    return UIntToVariant(value);
                case float value:
                    return FloatToVariant(value);
                case string value:
                    return StringToVariant(value);
                case HString value:
                    return HStringToVariant(value);
                case HArray value:
                    return HArrayToVariant(value);
                case ICollection value:
                    return ICollectionToVariant(value);
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
                    return NativeMarshal.InteropBoolToBool(variant.Bool);
                case VariantType.Int:
                    return variant.Int;
                case VariantType.UInt:
                    return variant.UInt;
                case VariantType.Float:
                    return variant.Float;
                case VariantType.String:
                    return NativeMarshal.HStringInternalToString(variant.String);
                case VariantType.Array:
                    return new HArray(variant.Array);
            }

            throw new NotImplementedException("C# Variant -> Object conversion not fully implemented");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant BoolToVariant(bool value)
            => new() { Type = VariantType.Bool, Bool = NativeMarshal.BoolToInteropBool(value) };

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant IntToVariant(long value)
            => new() { Type = VariantType.Int, Int = value };

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant UIntToVariant(ulong value)
            => new() { Type = VariantType.UInt, UInt = value };

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant FloatToVariant(float value)
            => new() { Type = VariantType.Float, Float = value };

        public static Variant StringToVariant(string value)
        {
            using HString hstr = new HString(value);
            return HStringToVariant(hstr);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant HStringToVariant(HString value)
        {
            Native_Variant_FromHString(out var variant, value._internalVal);
            return variant;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Variant HArrayToVariant(HArray value)
        {
            Native_Variant_FromHArray(out var variant, value._internalVal);
            return variant;
        }

        public static Variant ICollectionToVariant(ICollection value)
        {
            HArray harr = new HArray(value);
            return HArrayToVariant(harr);
        }

        [DllImport("__Internal")]
        internal static extern void Native_Variant_FromHArray(out Variant variant, in HArrayInternal value);

        [DllImport("__Internal")]
        internal static extern void Native_Variant_FromHString(out Variant variant, in HStringInternal value);
    }
}