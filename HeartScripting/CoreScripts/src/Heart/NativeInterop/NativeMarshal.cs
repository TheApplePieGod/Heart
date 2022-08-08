using Heart.Container;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public static class NativeMarshal
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static bool InteropBoolToBool(InteropBool value)
        { return value == InteropBool.True; }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static InteropBool BoolToInteropBool(bool value)
        { return value ? InteropBool.True : InteropBool.False; }

        internal static unsafe string HStringInternalToString(HStringInternal str)
        {
            if (!str.IsValid()) return "";

            if (str.Encoding == Encoding.UTF8)
                return Marshal.PtrToStringUTF8(new IntPtr(str.Data));
            else if (str.Encoding == Encoding.UTF16)
                return Marshal.PtrToStringUni(new IntPtr(str.Data));

            throw new NotImplementedException("C# HStringInternal -> String conversion not fully implemented");
        }

        internal static unsafe T[] PtrToArray<T>(T* src, uint count) where T : unmanaged
        {
            var ret = new T[count];
            fixed (T* ptr = ret)
            {
                for (uint i = 0; i < count; i++)
                    ptr[i] = src[i];
            }
            return ret;
        }

        internal static unsafe void CopyArrayToPtr<T>(T[] src, T* dst, uint count) where T : unmanaged
        {
            for (uint i = 0; i < count; i++)
                dst[i] = src[i];
        }
    }
}
