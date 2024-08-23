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
        public static bool IsButtonPressed(ButtonCode button)
            => NativeMarshal.InteropBoolToBool(Native_Input_IsButtonPressed((ushort)button));

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double GetAxisValue(AxisCode axis)
            => Native_Input_GetAxisValue((ushort)axis);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double GetAxisDelta(AxisCode axis)
            => Native_Input_GetAxisDelta((ushort)axis);

        [UnmanagedCallback]
        internal static partial InteropBool Native_Input_IsKeyPressed(ushort key);

        [UnmanagedCallback]
        internal static partial InteropBool Native_Input_IsButtonPressed(ushort button);

        [UnmanagedCallback]
        internal static partial double Native_Input_GetAxisValue(ushort axis);

        [UnmanagedCallback]
        internal static partial double Native_Input_GetAxisDelta(ushort axis);
    }
}
