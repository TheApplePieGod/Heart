using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    public enum Encoding : byte
    {
        UTF8 = 0,
        UTF16 = 1
    }

    [StructLayout(LayoutKind.Explicit, Size=16)]
    public unsafe struct HStringInternal
    {
        [FieldOffset(0)] public Encoding Encoding;
        [FieldOffset(8)] public void* Data;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public ContainerInfo* GetInfo()
            => (ContainerInfo*)Data - 1;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool IsValid()
            => Data != null;
    }

    // Todo: typed version
    public class HString : IDisposable
    {
        internal unsafe HStringInternal _internalVal;

        public HString(string value)
        {
            Native_HString_Init(out _internalVal, value);
        }

        internal HString(HStringInternal internalVal)
        {
            Native_HString_Copy(out _internalVal, internalVal);
        }

        ~HString()
        {
            Native_HString_Destroy(_internalVal);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            // Always destroy b/c we need to decrement the refcount
            Native_HString_Destroy(_internalVal);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal unsafe void CopyTo(HStringInternal* dst)
        {
            Native_HString_Copy(out *dst, _internalVal);
        }

        internal bool Valid
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internalVal.IsValid();
        }

        public Encoding Encoding
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _internalVal.Encoding;
        }

        public unsafe uint Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return Valid ? _internalVal.GetInfo()->ElemCount : 0; }
        }

        private unsafe uint RefCount
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return Valid ? _internalVal.GetInfo()->RefCount : 0; }
        }

        [DllImport("__Internal")]
        internal static extern void Native_HString_Init(out HStringInternal str, [MarshalAs(UnmanagedType.LPWStr)] string value);

        [DllImport("__Internal")]
        internal static extern void Native_HString_Destroy(in HStringInternal str);

        [DllImport("__Internal")]
        internal static extern void Native_HString_Copy(out HStringInternal dst, in HStringInternal src);
    }
}
