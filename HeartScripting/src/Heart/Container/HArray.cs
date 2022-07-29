﻿using System;
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

        internal HArray(HArrayInternal internalVal)
        {
            Native_HArray_Copy(ref _internalVal, ref internalVal);
        }

        ~HArray()
        {
            Native_HArray_Destroy(ref _internalVal);
        }

        public void Destroy()
        {
            GC.SuppressFinalize(this);
            Native_HArray_Destroy(ref _internalVal);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Add(object obj)
        {
            var variant = VariantConverter.ObjectToVariant(obj);
            Native_HArray_Add(ref _internalVal, ref variant);
        }

        public unsafe object this[int index]
        {
            get
            {
                if (index < 0 || index >= Count)
                    throw new IndexOutOfRangeException("HArray[] index out of range");
                Variant variant = _internalVal.Data[index];
                return VariantConverter.VariantToObject(variant);
            }
            set
            {
                if (index < 0 || index >= Count)
                    throw new IndexOutOfRangeException("HArray[] index out of range");
                _internalVal.Data[index] = VariantConverter.ObjectToVariant(value);
            }
        }

        public unsafe uint Count
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
        private static extern void Native_HArray_Init([In, Out] ref HArrayInternal array);

        [DllImport("__Internal")]
        private static extern void Native_HArray_Destroy([In] ref HArrayInternal array);

        [DllImport("__Internal")]
        private static extern void Native_HArray_Copy([In, Out] ref HArrayInternal dst, [In] ref HArrayInternal src);

        [DllImport("__Internal")]
        private static extern void Native_HArray_Add([In] ref HArrayInternal array, [In] ref Variant value);
    }
}
