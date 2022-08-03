using Heart.NativeInterop;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Input
{
    public static class Input
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool IsKeyPressed(KeyCode key)
            => NativeMarshal.InteropBoolToBool(Native_Input_IsKeyPressed((ushort)key));

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool IsMouseButtonPressed(MouseCode button)
            => NativeMarshal.InteropBoolToBool(Native_Input_IsMouseButtonPressed((ushort)button));

        [DllImport("__Internal")]
        internal static extern InteropBool Native_Input_IsKeyPressed(ushort key);

        [DllImport("__Internal")]
        internal static extern InteropBool Native_Input_IsMouseButtonPressed(ushort button);
    }
}
