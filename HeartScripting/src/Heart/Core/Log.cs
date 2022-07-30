using System.Runtime.InteropServices;

namespace Heart.Core
{
    public class Log
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

        public static void Trace(string format, params object[] args) => Native_Log(Level.Trace, string.Format(format, args));
        public static void Debug(string format, params object[] args) => Native_Log(Level.Debug, string.Format(format, args));
        public static void Info(string format, params object[] args) => Native_Log(Level.Info, string.Format(format, args));
        public static void Warn(string format, params object[] args) => Native_Log(Level.Warn, string.Format(format, args));
        public static void Error(string format, params object[] args) => Native_Log(Level.Error, string.Format(format, args));
        public static void Critical(string format, params object[] args) => Native_Log(Level.Critical, string.Format(format, args));

        public static void Trace(object value) => Native_Log(Level.Trace, value.ToString());
        public static void Debug(object value) => Native_Log(Level.Debug, value.ToString());
        public static void Info(object value) => Native_Log(Level.Info, value.ToString());
        public static void Warn(object value) => Native_Log(Level.Warn, value.ToString());
        public static void Error(object value) => Native_Log(Level.Error, value.ToString());
        public static void Critical(object value) => Native_Log(Level.Critical, value.ToString());

        [DllImport("__Internal")]
        internal static extern void Native_Log(Level level, [MarshalAs(UnmanagedType.LPStr)] string format);
    }
}
