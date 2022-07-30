using Heart.Container;
using System;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public static class NativeMarshal
    {
        internal static bool InteropBoolToBool(InteropBool value)
        { return value == InteropBool.True; }

        internal static InteropBool BoolToInteropBool(bool value)
        { return value ? InteropBool.True : InteropBool.False; }

        internal static unsafe string HStringInternalToString(HStringInternal str)
        {

            if (str.Encoding == Encoding.UTF8)
                return Marshal.PtrToStringUTF8(new IntPtr(str.Data));
            else if (str.Encoding == Encoding.UTF16)
                return Marshal.PtrToStringUni(new IntPtr(str.Data));

            throw new NotImplementedException("C# HStringInternal -> String conversion not fully implemented");
        }
    }
}
