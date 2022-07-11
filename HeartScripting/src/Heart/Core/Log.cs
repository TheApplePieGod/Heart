using System.Runtime.CompilerServices;

namespace Heart.Core
{
    public class Log
    {
        internal enum Level : int
        {
            Trace = (1 << 0),
            Debug = (1 << 1),
            Info = (1 << 2),
            Warn = (1 << 3),
            Error = (1 << 4),
            Critical = (1 << 5)
        }

        public static void Trace(string format, params object[] args) => Log_Native(Level.Trace, string.Format(format, args));
        public static void Debug(string format, params object[] args) => Log_Native(Level.Debug, string.Format(format, args));
        public static void Info(string format, params object[] args) => Log_Native(Level.Info, string.Format(format, args));
        public static void Warn(string format, params object[] args) => Log_Native(Level.Warn, string.Format(format, args));
        public static void Error(string format, params object[] args) => Log_Native(Level.Error, string.Format(format, args));
        public static void Critical(string format, params object[] args) => Log_Native(Level.Critical, string.Format(format, args));

        public static void Trace(object value) => Log_Native(Level.Trace, value.ToString());
        public static void Debug(object value) => Log_Native(Level.Debug, value.ToString());
        public static void Info(object value) => Log_Native(Level.Info, value.ToString());
        public static void Warn(object value) => Log_Native(Level.Warn, value.ToString());
        public static void Error(object value) => Log_Native(Level.Error, value.ToString());
        public static void Critical(object value) => Log_Native(Level.Critical, value.ToString());

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void Log_Native(Level level, string format);
    }
}
