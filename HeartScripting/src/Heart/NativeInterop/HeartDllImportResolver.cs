using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public class HeartDllImportResolver
    {
        private IntPtr nativeDllHandle;

        public HeartDllImportResolver(IntPtr nativeDllHandle)
        {
            this.nativeDllHandle = nativeDllHandle;
        }

        public IntPtr OnResolveDllImport(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {
            // __Internal refers to exports from the main native process whose handle
            // was passed in via the constructor
            if (libraryName == "__Internal")
                return nativeDllHandle;

            return IntPtr.Zero;
        }
    }
}
