#pragma once

namespace Heart
{
    // Todo: thread-safety
    template <typename T>
    class Container
    {
    public:
        Container() = default;

        Container(const Container& other)
        {
            m_Data = other.m_Data;
            IncrementRefCount();
        }

        Container(u32 elemCount)
        {
            Resize(elemCount, true);
            IncrementRefCount();
        }

        Container(T* data, u32 dataCount)
        {
            Resize(dataCount, false);
            memcpy(m_Data, data, dataCount * sizeof(T));
            IncrementRefCount();
        }

        Container(std::initializer_list<T> list)
        {
            Resize(list.size(), false);
            memcpy(m_Data, list.begin(), list.size() * sizeof(T));
            IncrementRefCount();
        }

        ~Container()
        {
            if (!m_Data) return;
            Free();
        }

        inline void Clear(bool shrink = false)
        {
            SetCount(0);
            if (shrink) Resize(0);
        }

        // This WILL shrink the memory
        inline void Resize(u32 elemCount, bool construct = true)
        {
            ResizeExplicit(elemCount, GetNextPowerOfTwo(elemCount), construct);
        }

        inline Container Clone() { return Container(m_Data, GetCount()); }
        inline u32 GetCount() { return m_Data ? *GetCountPtr() : 0; }
        inline u32 GetAllocatedCount() { return m_Data ? *GetAllocatedCountPtr() : 0; }
        inline u32 IncrementCount() { return ++*GetCountPtr(); }
        inline u32 DecrementCount() { return --*GetCountPtr(); }
        inline T* Begin() { return m_Data; }
        inline T* End() { return m_Data + GetCount(); }
        inline T& Get(u32 index) { return m_Data[index]; }
        inline T& operator[](u32 index) { return m_Data[index]; }

    private:

        // Value must not be zero
        u32 GetNextPowerOfTwo(u32 value)
        {
            // Minimum alloc size of 16 elems
            if (value <= 16) return 16;
            u32 two = value - 1;
            two |= two >> 1;
            two |= two >> 2;
            two |= two >> 4;
            two |= two >> 8;
            two |= two >> 16;
            return ++two;
        }

        void ResizeExplicit(u32 elemCount, u32 allocCount, bool construct = true)
        {
            if (elemCount == 0 && m_Data)
            {
                Free();
                return;
            }

            u32 oldCount = GetCount();
            if (elemCount < oldCount)
                *GetCountPtr() = elemCount;
            
            u32* data = reinterpret_cast<u32*>(::operator new(allocCount * sizeof(T) + sizeof(u32) * 3));
            *data = GetRefCount();
            *(data + 1) = allocCount;
            *(data + 2) = elemCount;
            T* newData = reinterpret_cast<T*>(data + 3);

            if (m_Data)
            {
                memcpy(newData, m_Data, oldCount * sizeof(T));
                Free();
            }
            
            m_Data = newData;

            if (construct && std::is_trivially_constructible<T>::value)
                for (u32 i = oldCount; i < elemCount; i++)
                    HE_PLACEMENT_NEW(m_Data + i, T);
        }

        void Free()
        {
            if (DecrementRefCount() == 0)
            {
                // Destruct
                u32 count = GetCount();
                if (std::is_trivially_destructible<T>::value)
                    for (u32 i = 0; i < count; i++)
                        m_Data[i].~T();
                    
                // Delete initial (origin) pointer
                ::operator delete[](reinterpret_cast<u32*>(m_Data) - 3);
            }
            m_Data = nullptr;
        }

        inline u32* GetCountPtr() { return reinterpret_cast<u32*>(m_Data) - 1; }
        inline u32* GetAllocatedCountPtr() { return reinterpret_cast<u32*>(m_Data) - 2; }
        inline u32* GetRefCountPtr() { return reinterpret_cast<u32*>(m_Data) - 3; }
        inline u32 GetRefCount() { return m_Data ? *GetRefCountPtr() : 1; } // Default one for this obj
        inline u32 IncrementRefCount() { return ++*GetRefCountPtr(); }
        inline u32 DecrementRefCount() { return --*GetRefCountPtr(); }
        inline void SetCount(u32 count) { *GetCountPtr() = count; }

    private:
        T* m_Data = nullptr;
    };
}