#pragma once

#include "Heart/Container/Container.hpp"

namespace Heart
{
    template <typename T>
    class HVector
    {
    public:
        HVector() = default;
        ~HVector() = default;

        HVector(const HVector& other)
            : m_Container(other.m_Container)
        {}

        HVector(HVector&& other)
            : m_Container(std::move(other.m_Container))
        {}

        HVector(u32 elemCount, bool fill = true)
            : m_Container(elemCount, fill)
        {}

        HVector(T* data, u32 dataCount)
            : m_Container(data, dataCount)
        {}

        HVector(std::initializer_list<T> list)
            : m_Container(list)
        {}

        HVector(T* start, T* end)
            : m_Container(start, end - start)
        {}

        void Add(const T& elem)
        {
            // Placement new here prevents accidental double destruction
            u32 addIndex = PreAdd();
            HE_PLACEMENT_NEW(Begin() + addIndex, T, elem);
        }

        template <class... Args>
        void AddInPlace(Args... args)
        {
            u32 addIndex = PreAdd();
            HE_PLACEMENT_NEW(
                Begin() + addIndex,
                T,
                std::forward<Args>(args)...
            );
        }

        void Remove(u32 index)
        {
            if (index >= Count()) throw std::out_of_range("Called Remove() on container with out of range index");

            // Destruct
            if constexpr (m_ShouldDestruct)
                m_Container[index].~T();

            if (m_Container.DecrementCount() == 0 || index == Count()) return;

            memmove(
                Begin() + index,
                Begin() + index + 1,
                (Count() - index) * sizeof(T)
            );
        }

        void RemoveUnordered(u32 index)
        {
            if (index >= Count()) throw std::out_of_range("Called Remove() on container with out of range index");

            // Destruct
            if constexpr (m_ShouldDestruct)
                m_Container[index].~T();

            if (m_Container.DecrementCount() == 0) return;
            
            memmove(
                Begin() + index,
                Begin() + Count(),
                sizeof(T)
            );
        }

        void Pop()
        {
            if (Count() == 0) throw std::out_of_range("Called Pop() on container with a count of zero");

            // Destruct
            if constexpr (m_ShouldDestruct)
                m_Container.Data()[m_Container.DecrementCount()].~T();
            else
                m_Container.DecrementCount();
        }

        void CopyFrom(const T* start, const T* end)
        {
            m_Container = Container(start, end - start);
        }

        void Insert(const HVector<T>& other, u32 index, bool shallow = false)
        {
            u32 oldCount = Count();
            u32 otherCount = other.Count();
            index = std::min(index, oldCount);

            Resize(oldCount + otherCount, false);

            memmove(
                Begin() + index + otherCount,
                Begin() + index,
                sizeof(T) * oldCount - index
            );
            
            if (shallow)
                memcpy(Data() + index, other.Begin(), otherCount * sizeof(T));
            else
            {
                for (u32 i = 0; i < other.Count(); i++)
                    HE_PLACEMENT_NEW(Begin() + i + index, T, other[i]);
            }
        }

        void Insert(const T& other, u32 index)
        {
            u32 oldCount = PreAdd();

            // Shift elements
            index = std::min(index, oldCount);
            if (index != oldCount)
            {
                memmove(
                    Begin() + index + 1,
                    Begin() + index,
                    (oldCount - index) * sizeof(T)
                );
            }

            HE_PLACEMENT_NEW(Begin() + index, T, other);
        }

        void Append(const HVector<T>& other, bool shallow = false)
        {
            Insert(other, Count(), shallow);
        }

        // find
        
        inline void Reserve(u32 allocCount) { m_Container.Reserve(allocCount); }
        inline void Clear(bool shrink = false) { m_Container.Clear(shrink); }
        inline void Trim(u32 newCount) { m_Container.Trim(newCount); }
        inline void Resize(u32 elemCount, bool construct = true) { m_Container.Resize(elemCount, construct); }
        inline HVector Clone() const { return HVector(m_Container.Clone()); }
        inline HVector& CloneInPlace() { m_Container = Container(Data(), Count()); return *this; }
        inline void ShallowCopy(const HVector& from) { m_Container.Copy(from.m_Container, true); }
        inline u32 Count() const { return m_Container.Count(); }
        inline u32 GetAllocatedCount() const { return m_Container.GetAllocatedCount(); }
        inline u32 GetRefCount() const { return m_Container.GetRefCount(); }
        inline T* Data() const { return m_Container.Data(); }
        inline T* Begin() const { return m_Container.Begin(); }
        inline T* End() const { return m_Container.End(); }
        inline T& Front() const { return *m_Container.Begin(); }
        inline T& Back() const { return *(m_Container.End() - 1); }
        inline T& Get(u32 index) const { return m_Container.Get(index); }
        inline bool IsEmpty() const { return m_Container.IsEmpty(); }

        inline T& operator[](u32 index) const { return m_Container[index]; }
        inline void operator=(const HVector& other) { m_Container = other.m_Container; }

        // For range loops
        inline T* begin() const { return Begin(); }
        inline T* end() const { return End(); }

    private:
        HVector(const Container<T>& container)
            : m_Container(container)
        {}

        u32 PreAdd()
        {
            u32 count = Count();
            if (count >= m_Container.GetAllocatedCount())
                m_Container.Resize(count + 1, false);
            else
                m_Container.IncrementCount();
            return count;
        }
        
        inline static constexpr bool m_ShouldDestruct = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;

    private:
        Container<T> m_Container;
    };
}
