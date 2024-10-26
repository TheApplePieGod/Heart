using System;

namespace Heart.NativeBridge
{
    [AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    internal sealed class UnmanagedCallbackAttribute : Attribute
    {
        public UnmanagedCallbackAttribute()
        {

        }
    }
}
