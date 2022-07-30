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

        HVector(u32 elemCount)
            : m_Container(elemCount)
        {}

        HVector(T* data, u32 dataCount)
            : m_Container(data, dataCount)
        {}

        HVector(std::initializer_list<T> list)
            : m_Container(list)
        {}

        void Add(const T& elem)
        {
            u32 addIndex = PreAdd();
            m_Container[addIndex] = elem;
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

            if (m_Container.DecrementCount() == 0) return;

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

        // insert
        // find
        
        inline void Reserve(u32 allocCount) { m_Container.Reserve(allocCount); }
        inline void Clear(bool shrink = false) { m_Container.Clear(shrink); }
        inline void Resize(u32 elemCount, bool construct = true) { m_Container.Resize(elemCount, construct); }
        inline HVector Clone() { return HVector(m_Container.Clone()); }
        inline u32 GetCount() { return m_Container.GetCount(); }
        inline T* Data() { return m_Container.Data(); }
        inline T* Begin() { return m_Container.Begin(); }
        inline T* End() { return m_Container.End(); }
        inline T* Front() { return m_Container.Begin(); }
        inline T* Back() { return GetCount() > 0 ? m_Container.End() - 1 : m_Container.Begin(); }
        inline T& Get(u32 index) { return m_Container.Get[index]; }

        inline T& operator[](u32 index) { return m_Container[index]; }
        inline void operator=(const HVector& other) { HE_PLACEMENT_NEW(&m_Container, Container<T>, other.m_Container); }

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