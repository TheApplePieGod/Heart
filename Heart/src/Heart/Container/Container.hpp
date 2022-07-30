#pragma once

namespace Heart
{
    // Todo: thread-safety
    template <typename T>
    class Container
    {
    public:
        Container() = default;

        Container(const Container<T>& other)
        { Copy(other); }

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
            memcpy(m_Data, data, dataCount * sizeof(T));
        }

        Container(std::initializer_list<T> list)
        {
            Resize(list.size(), false);
            memcpy(m_Data, list.begin(), list.size() * sizeof(T));
        }

        ~Container()
        {
            if (!m_Data) return;
            Cleanup();
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

        void Reserve(u32 allocCount)
        {
            u32 actualAlloc = GetNextPowerOfTwo(allocCount);
            if (actualAlloc > GetAllocatedCount())
                ResizeExplicit(GetCount(), actualAlloc, false);
        }

        inline Container Clone() const { return Container(m_Data, GetCount()); }
        inline u32 GetCount() const { return m_Data ? GetCountUnchecked() : 0; }
        inline u32 GetCountUnchecked() const { return GetInfoPtr()->ElemCount; }
        inline u32 GetAllocatedCount() const { return m_Data ? GetInfoPtr()->AllocatedCount : 0; }
        inline u32 IncrementCount() { return ++GetInfoPtr()->ElemCount; }
        inline u32 DecrementCount() { return --GetInfoPtr()->ElemCount; }
        inline T* Data() const { return m_Data; }
        inline T* Begin() const{ return m_Data; }
        inline T* End() const { return m_Data + GetCount(); }
        inline T& Get(u32 index) const{ return m_Data[index]; }

        inline T& operator[](u32 index) const { return m_Data[index]; }
        inline void operator=(const Container<T>& other) { Copy(other); }

    private:
        struct ContainerInfo
        {
            u32 RefCount;
            u32 AllocatedCount;
            u32 ElemCount;
        };

    private:
        void Copy(const Container<T>& other)
        {
            if (!other.m_Data) return;
            m_Data = other.m_Data;
            IncrementRefCount();
        }

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
                Cleanup();
                return;
            }

            u32 oldCount = GetCount();
            if (elemCount < oldCount)
            {
                // Destruct removed elements
                if (ShouldDestruct())
                    for (u32 i = elemCount; i < oldCount; i++)
                        m_Data[i].~T();

                SetCount(elemCount);
            }
            
            ContainerInfo* data = reinterpret_cast<ContainerInfo*>(::operator new(allocCount * sizeof(T) + sizeof(ContainerInfo)));
            data->RefCount = GetRefCount();
            data->AllocatedCount = allocCount;
            data->ElemCount = elemCount;
            T* newData = reinterpret_cast<T*>(data + 1);
    
            if (m_Data)
            {
                memcpy(newData, m_Data, oldCount * sizeof(T));
                FreeMemory();
            }
            
            m_Data = newData;

            if (construct && ShouldConstruct())
                for (u32 i = oldCount; i < elemCount; i++)
                    HE_PLACEMENT_NEW(m_Data + i, T);
        }

        void FreeMemory()
        {
            // Delete initial (origin) pointer
            ::operator delete[](reinterpret_cast<ContainerInfo*>(m_Data) - 1);
        }

        void Cleanup()
        {
            if (DecrementRefCount() == 0)
            {
                // Destruct
                u32 count = GetCount();
                if (ShouldDestruct())
                    for (u32 i = 0; i < count; i++)
                        m_Data[i].~T();
                    
                FreeMemory();

                // static int freed = 0;
                // HE_ENGINE_LOG_WARN("Freed {0} on thread {1}", ++freed, std::hash<std::thread::id>{}(std::this_thread::get_id()));
            }
            m_Data = nullptr;
        }

        inline ContainerInfo* GetInfoPtr() const { return reinterpret_cast<ContainerInfo*>(m_Data) - 1; }
        inline u32 GetRefCount() { return m_Data ? GetInfoPtr()->RefCount : 1; } // Default one for this obj
        inline u32 IncrementRefCount() { return ++GetInfoPtr()->RefCount; }
        inline u32 DecrementRefCount() { return --GetInfoPtr()->RefCount; }
        inline void SetCount(u32 count) { GetInfoPtr()->ElemCount = count; }
        inline constexpr bool ShouldDestruct() const { return std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value; }
        inline constexpr bool ShouldConstruct() const { return std::is_default_constructible<T>::value; }

    private:
        T* m_Data = nullptr;
    };
}