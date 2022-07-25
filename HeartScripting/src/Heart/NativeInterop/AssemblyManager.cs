using Heart.Core;
using Heart.NativeInterop;
using System;
using System.Runtime.InteropServices;

namespace Heart.NativeInterop
{
    public static class AssemblyManager
    {
        [UnmanagedCallersOnly]
        public static InteropBool LoadAssembly([In] IntPtr assemblyNameStr)
        {
            string s = Marshal.PtrToStringUTF8(assemblyNameStr);

            Console.WriteLine(s);
            return InteropBool.True;
        }
    }
}
