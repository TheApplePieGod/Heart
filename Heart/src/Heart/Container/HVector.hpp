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
            HE_PLACEMENT_NEW(m_Container.Begin() + addIndex, T, elem);
        }

        template <class... Args>
        void AddInPlace(Args... args)
        {
            u32 addIndex = PreAdd();
            HE_PLACEMENT_NEW(
                m_Container.Begin() + addIndex,
                T,
                std::forward<Args>(args)...
            );
        }

        void Remove(u32 index)
        {
            if (index >= GetCount()) throw std::out_of_range("Called Remove() on container with out of range index");

            // Destruct
            if (ShouldDestruct())
                m_Container[index].~T();

            if (m_Container.DecrementCount() == 0 || index == GetCount()) return;

            memmove(
                m_Container.Begin() + index,
                m_Container.Begin() + index + 1,
                (GetCount() - index) * sizeof(T)
            );
        }

        void RemoveUnordered(u32 index)
        {
            if (index >= GetCount()) throw std::out_of_range("Called Remove() on container with out of range index");

            // Destruct
            if (ShouldDestruct())
                m_Container[index].~T();

            if (m_Container.DecrementCount() == 0) return;
            
            m_Container[index] = m_Container[GetCount()];
        }

        void Pop()
        {
            if (GetCount() == 0) throw std::out_of_range("Called Pop() on container with a count of zero");

            // Destruct
            if (ShouldDestruct())
                m_Container[m_Container.DecrementCount()].~T();
        }

        void CopyFrom(const T* start, const T* end)
        {
            m_Container = Container(start, end - start);
        }

        void Append(const HVector<T>& other, bool shallow = false)
        {
            u32 oldCount = GetCount();
            Resize(oldCount + other.GetCount(), false);
            if (shallow)
                memcpy(End(), other.Begin(), other.GetCount() * sizeof(T));
            else
            {
                for (u32 i = 0; i < other.GetCount(); i++)
                    HE_PLACEMENT_NEW(Begin() + i + oldCount, T, other[i]);
            }
        }

        // insert
        // find
        
        inline void Reserve(u32 allocCount) { m_Container.Reserve(allocCount); }
        inline void Clear(bool shrink = false) { m_Container.Clear(shrink); }
        inline void Resize(u32 elemCount, bool construct = true) { m_Container.Resize(elemCount, construct); }
        inline HVector Clone() const { return HVector(m_Container.Clone()); }
        inline HVector& CloneInPlace() { m_Container = Container(Data(), GetCount()); return *this; }
        inline u32 GetCount() const { return m_Container.GetCount(); }
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
            u32 count = GetCount();
            if (count >= m_Container.GetAllocatedCount())
                m_Container.Resize(count + 1, false);
            else
                m_Container.IncrementCount();
            return count;
        }
        
        inline constexpr bool ShouldDestruct() const { return std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value; }

    private:
        Container<T> m_Container;
    };
}