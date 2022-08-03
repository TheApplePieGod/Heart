using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public class HeartDllImportResolver
    {
        private IntPtr _nativeDllHandle;

        public HeartDllImportResolver(IntPtr nativeDllHandle)
        {
            _nativeDllHandle = nativeDllHandle;
        }

        public IntPtr OnResolveDllImport(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {
            // __Internal refers to exports from the main native process whose handle
            // was passed in via the constructor
            if (libraryName == "__Internal")
                return _nativeDllHandle;

            return IntPtr.Zero;
        }
    }
}
