using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Heart.Container
{
    [StructLayout(LayoutKind.Explicit, Size=8)]
    internal unsafe struct HArrayInternal
    {
        [FieldOffset(0)] public Variant* Data;
    }

    // Todo: typed version
    public sealed class HArray : IList<object>, ICollection, IDisposable
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

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            // Always destroy b/c we need to decrement the refcount
            Native_HArray_Destroy(ref _internalVal);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Add(object obj)
        {
            using var variant = VariantConverter.ObjectToVariant(obj);
            Native_HArray_Add(ref _internalVal, variant);
        }

        public void CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }

        public int IndexOf(object item)
        {
            throw new NotImplementedException();
        }

        public void Insert(int index, object item)
        {
            throw new NotImplementedException();
        }

        public void RemoveAt(int index)
        {
            throw new NotImplementedException();
        }

        public void Clear()
        {
            throw new NotImplementedException();
        }

        public bool Contains(object item)
        {
            throw new NotImplementedException();
        }

        public void CopyTo(object[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public bool Remove(object item)
        {
            throw new NotImplementedException();
        }

        public IEnumerator GetEnumerator()
        {
            throw new NotImplementedException();
        }

        IEnumerator<object> IEnumerable<object>.GetEnumerator()
        {
            throw new NotImplementedException();
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
                _internalVal.Data[index].Dispose();
                _internalVal.Data[index] = VariantConverter.ObjectToVariant(value);
            }
        }

        public bool IsSynchronized => false;
        public object SyncRoot => null;

        public unsafe int Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return (int)GetInfo()->ElemCount; }
        }

        private unsafe uint RefCount
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return GetInfo()->RefCount; }
        }

        public bool IsReadOnly => throw new NotImplementedException();

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ContainerInfo* GetInfo()
        {
            return (ContainerInfo*)_internalVal.Data - 1;
        }

        [DllImport("__Internal")]
        internal static extern void Native_HArray_Init([In, Out] ref HArrayInternal array);

        [DllImport("__Internal")]
        internal static extern void Native_HArray_Destroy([In] ref HArrayInternal array);

        [DllImport("__Internal")]
        internal static extern void Native_HArray_Copy([In, Out] ref HArrayInternal dst, [In] ref HArrayInternal src);

        [DllImport("__Internal")]
        internal static extern void Native_HArray_Add([In] ref HArrayInternal array, [In] Variant value);
    }
}
