using Heart.NativeInterop;
using Heart.NativeBridge;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Input
{
    public static partial class Input
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool IsKeyPressed(KeyCode key)
            => NativeMarshal.InteropBoolToBool(Native_Input_IsKeyPressed((ushort)key));

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool IsMouseButtonPressed(MouseCode button)
            => NativeMarshal.InteropBoolToBool(Native_Input_IsMouseButtonPressed((ushort)button));

        [UnmanagedCallback]
        internal static partial InteropBool Native_Input_IsKeyPressed(ushort key);

        [UnmanagedCallback]
        internal static partial InteropBool Native_Input_IsMouseButtonPressed(ushort button);
    }
}
