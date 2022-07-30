using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    public enum Encoding : byte
    {
        UTF8 = 0,
        UTF16
    }

    [StructLayout(LayoutKind.Explicit, Size=16)]
    internal unsafe struct HStringInternal
    {
        [FieldOffset(0)] public Encoding Encoding;
        [FieldOffset(8)] public void* Data;
    }

    // Todo: typed version
    public class HString : IDisposable
    {
        internal unsafe HStringInternal _internalVal;

        public HString(string value)
        {
            Native_HString_Init(ref _internalVal, value);
        }

        internal HString(HStringInternal internalVal)
        {
            Native_HString_Copy(ref _internalVal, ref internalVal);
        }

        ~HString()
        {
            Native_HString_Destroy(ref _internalVal);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            // Always destroy b/c we need to decrement the refcount
            Native_HString_Destroy(ref _internalVal);
        }

        public Encoding Encoding
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internalVal.Encoding;
        }

        public unsafe uint Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return GetInfo()->ElemCount; }
        }

        private unsafe uint RefCount
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return GetInfo()->RefCount; }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ContainerInfo* GetInfo()
        {
            return (ContainerInfo*)_internalVal.Data - 1;
        }

        [DllImport("__Internal")]
        internal static extern void Native_HString_Init([In, Out] ref HStringInternal str, [MarshalAs(UnmanagedType.LPWStr)] [In] string value);

        [DllImport("__Internal")]
        internal static extern void Native_HString_Destroy([In] ref HStringInternal str);

        [DllImport("__Internal")]
        internal static extern void Native_HString_Copy([In, Out] ref HStringInternal dst, [In] ref HStringInternal src);
    }
}
