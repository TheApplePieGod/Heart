using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct HArrayInternal
    {
        public Variant* Data;
    }

    // Todo: typed version
    public class HArray
    {
        internal unsafe HArrayInternal _internalVal;

        public HArray()
        {
            Native_HArray_Init(ref _internalVal);
        }

        ~HArray()
        {
            Native_HArray_Free(ref _internalVal);
        }

        public void Free()
        {
            Native_HArray_Free(ref _internalVal);
            GC.SuppressFinalize(this);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Add(object obj)
        {
            var variant = VariantConverter.ObjectToVariant(obj);
            Native_HArray_Add(ref _internalVal, ref variant);
        }

        public unsafe uint Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return GetInfo()->ElemCount; }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ContainerInfo* GetInfo()
        {
            return (ContainerInfo*)_internalVal.Data - 1;
        }

        [DllImport("__Internal")]
        private static extern void Native_HArray_Init([In, Out] ref HArrayInternal array);
        [DllImport("__Internal")]
        private static extern void Native_HArray_Free([In] ref HArrayInternal array);

        [DllImport("__Internal")]
        private static extern void Native_HArray_Add([In] ref HArrayInternal array, [In] ref Variant value);
    }
}
