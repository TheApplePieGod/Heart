#pragma once

namespace Heart
{
    // Todo: thread-safety
    template <typename T>
    class Container
    {
    public:
        Container() = default;

        Container(const Container<T>& other, bool shallow = false)
        { Copy(other, shallow); }

        Container(u32 elemCount, bool fill = true)
        {
            if (fill)
                Resize(elemCount, true);
            else
                Reserve(elemCount);
        }

        Container(const T* data, u32 dataCount)
        {
            Resize(dataCount, false);
            for (u32 i = 0; i < dataCount; i++)
                HE_PLACEMENT_NEW(m_Data + i, T, data[i]);
        }

        Container(std::initializer_list<T> list)
        {
            Resize(list.size(), false);
            for (u32 i = 0; i < Count(); i++)
                HE_PLACEMENT_NEW(m_Data + i, T, list.begin()[i]);
        }

        ~Container()
        {
            Cleanup();
        }

        void Copy(const Container<T>& other, bool shallow = false)
        {
            Cleanup();
            if (shallow)
            {
                m_Data = other.m_Data;
                if (!other.m_Data) return;
                IncrementRefCount();
                return;
            }
            if (other.Count() == 0) return;
            Resize(other.Count(), false);
            for (u32 i = 0; i < other.Count(); i++)
                HE_PLACEMENT_NEW(m_Data + i, T, other[i]);
        }

        void Clear(bool shrink = false)
        {
            if (Count() == 0) return;

            if (shrink)
            {
                Resize(0);
                return;
            }

            // Destruct removed elements
            if constexpr (m_ShouldDestruct)
            {
                for (u32 i = 0; i < Count(); i++)
                    m_Data[i].~T();
            }

            SetCount(0);
        }

        void Reserve(u32 allocCount)
        {
            u32 actualAlloc = GetNextPowerOfTwo(allocCount);
            if (actualAlloc > GetAllocatedCount())
                ResizeExplicit(Count(), actualAlloc, false);
        }

        // This WILL shrink the memory
        inline void Resize(u32 elemCount, bool construct = true)
        {
            ResizeExplicit(elemCount, GetNextPowerOfTwo(elemCount), construct);
        }

        inline Container Clone() const { return Container(m_Data, Count()); }
        inline u32 Count() const { return m_Data ? CountUnchecked() : 0; }
        inline u32 CountUnchecked() const { return GetInfoPtr()->ElemCount; }
        inline u32 GetAllocatedCount() const { return m_Data ? GetInfoPtr()->AllocatedCount : 0; }
        inline u32 GetRefCount() const { return m_Data ? GetInfoPtr()->RefCount : 1; } // Default is 1 for this obj
        inline u32 IncrementCount() { return ++GetInfoPtr()->ElemCount; }
        inline u32 DecrementCount() { return --GetInfoPtr()->ElemCount; }
        inline T* Data() const { return m_Data; }
        inline T* Begin() const{ return m_Data; }
        inline T* End() const { return m_Data + Count(); }
        inline bool IsEmpty() const { return Count() == 0; }
        inline T& Get(u32 index) const
        { HE_ENGINE_ASSERT(index < Count(), "Container access out of bounds"); return m_Data[index]; }

        inline T& operator[](u32 index) const { return Get(index); }
        inline void operator=(const Container<T>& other) { Copy(other); }

        inline static constexpr u32 MinimumAllocCount = 16;

    private:
        struct ContainerInfo
        {
            u32 RefCount;
            u32 AllocatedCount;
            u32 ElemCount;
        };

    private:
        // Value must not be zero
        u32 GetNextPowerOfTwo(u32 value)
        {
            // Minimum alloc amount
            if (value == 0) return 0;
            if (value <= MinimumAllocCount) return MinimumAllocCount;
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
            if (allocCount == 0)
            {
                Cleanup();
                return;
            }

            u32 oldCount = Count();
            if (elemCount < oldCount)
            {
                // Destruct removed elements
                if constexpr (m_ShouldDestruct)
                    for (u32 i = elemCount; i < oldCount; i++)
                        m_Data[i].~T();
            }
            
            if (allocCount != GetAllocatedCount())
            {
                ContainerInfo* data = reinterpret_cast<ContainerInfo*>(::operator new(allocCount * sizeof(T) + sizeof(ContainerInfo)));
                data->RefCount = GetRefCount();
                data->AllocatedCount = allocCount;
                T* newData = reinterpret_cast<T*>(data + 1);
        
                if (m_Data)
                {
                    memcpy(newData, m_Data, oldCount * sizeof(T));
                    FreeMemory();
                }
                
                m_Data = newData;
            }
            SetCount(elemCount);

            if (construct && elemCount > oldCount)
            {
                if constexpr (m_ShouldConstruct)
                {
                    for (u32 i = oldCount; i < elemCount; i++)
                        HE_PLACEMENT_NEW(m_Data + i, T);
                }
                else
                    memset(m_Data + oldCount, 0, (elemCount - oldCount) * sizeof(T));
            }
        }

        void FreeMemory()
        {
            // Delete initial (origin) pointer
            ::operator delete[](reinterpret_cast<ContainerInfo*>(m_Data) - 1);
        }

        void Cleanup()
        {
            if (!m_Data) return;
            if (DecrementRefCount() == 0)
            {
                // Destruct
                u32 count = Count();
                if constexpr (m_ShouldDestruct)
                    for (u32 i = 0; i < count; i++)
                        m_Data[i].~T();
                    
                FreeMemory();
            }
            m_Data = nullptr;
        }

        inline ContainerInfo* GetInfoPtr() const { return reinterpret_cast<ContainerInfo*>(m_Data) - 1; }
        inline u32 IncrementRefCount() { return ++GetInfoPtr()->RefCount; }
        inline u32 DecrementRefCount() { return --GetInfoPtr()->RefCount; }
        inline void SetCount(u32 count) { GetInfoPtr()->ElemCount = count; }
        inline static constexpr bool m_ShouldDestruct = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
        inline static constexpr bool m_ShouldConstruct = std::is_default_constructible<T>::value;

    private:
        T* m_Data = nullptr;
    };
}