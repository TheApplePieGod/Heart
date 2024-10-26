using System.Runtime.CompilerServices;
using Heart.NativeBridge;

namespace Heart.Core
{
    public partial class Log
    {
        internal enum Level : int
        {
            Trace = 0,
            Debug = 1,
            Info = 2,
            Warn = 3,
            Error = 4,
            Critical = 5
        }

        public static void Trace(string format, params object[] args) => CallNativeLog(Level.Trace, string.Format(format, args));
        public static void Debug(string format, params object[] args) => CallNativeLog(Level.Debug, string.Format(format, args));
        public static void Info(string format, params object[] args) => CallNativeLog(Level.Info, string.Format(format, args));
        public static void Warn(string format, params object[] args) => CallNativeLog(Level.Warn, string.Format(format, args));
        public static void Error(string format, params object[] args) => CallNativeLog(Level.Error, string.Format(format, args));
        public static void Critical(string format, params object[] args) => CallNativeLog(Level.Critical, string.Format(format, args));

        public static void Trace(object value) => CallNativeLog(Level.Trace, value.ToString());
        public static void Debug(object value) => CallNativeLog(Level.Debug, value.ToString());
        public static void Info(object value) => CallNativeLog(Level.Info, value.ToString());
        public static void Warn(object value) => CallNativeLog(Level.Warn, value.ToString());
        public static void Error(object value) => CallNativeLog(Level.Error, value.ToString());
        public static void Critical(object value) => CallNativeLog(Level.Critical, value.ToString());

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static unsafe void CallNativeLog(Level level, string message)
        {
            fixed (char* ptr = message)
            {
                Native_Log(level, ptr, (uint)message.Length);
            }
        }

        [UnmanagedCallback]
        internal static unsafe partial void Native_Log(Level level, char* message, uint messageLen);
    }
}
