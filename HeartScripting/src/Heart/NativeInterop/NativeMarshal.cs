using Heart.Container;

namespace Heart.NativeInterop
{
    public static class NativeMarshal
    {
        public static bool InteropBoolToBool(in InteropBool value)
        { return value == InteropBool.True; }

        public static InteropBool BoolToInteropBool(in bool value)
        { return value ? InteropBool.True : InteropBool.False; }
    }
}
