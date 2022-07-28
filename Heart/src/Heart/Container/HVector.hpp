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
        {
            m_Container = Container(other.m_Container);
        }

        HVector(u32 elemCount)
        {
            m_Container = Container<T>(elemCount);
        }

        HVector(T* data, u32 dataCount)
        {
            m_Container = Container(data, dataCount);
        }

        HVector(std::initializer_list<T> list)
        {
            m_Container = Container(list);
        }

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
            if (GetCount() == 0) throw std::out_of_range("Called Remove() on container with a count of zero");

            m_Container.DecrementCount();

            if (GetCount() == 0) return;

            // Destruct
            if (std::is_trivially_destructible<T>::value)
                m_Container[index].~T();

            memmove(
                m_Container.Begin() + index,
                m_Container.Begin() + index + 1,
                (GetCount() - index) * sizeof(T)
            );
        }

        void RemoveUnordered(u32 index)
        {
            if (GetCount() == 0) throw std::out_of_range("Called Remove() on container with a count of zero");

            m_Container.DecrementCount();

            if (GetCount() == 0) return;
            
            m_Container[index] = m_Container[GetCount()];
        }

        T Pop()
        {
            if (GetCount() == 0) throw std::out_of_range("Called Pop() on container with a count of zero");
            return m_Container[m_Container.DecrementCount()];
        }

        // insert
        // find
        
        inline void Clear(bool shrink = false) { m_Container.Clear(shrink); }
        inline void Resize(u32 elemCount, bool construct = true) { m_Container.Resize(elemCount, construct); }
        inline HVector Clone() { return HVector(m_Container); }
        inline u32 GetCount() { return m_Container.GetCount(); }
        inline u32 Begin() { return m_Container.Begin(); }
        inline u32 End() { return m_Container.End(); }
        inline T& operator[](u32 index) { return m_Container[index]; }
        inline T& Get(u32 index) { return m_Container.Get[index]; }

    private:
        HVector(const Container<T>& container)
        {
            HVector vec;
            vec.m_Container = container.Clone();
            return vec;
        }

        u32 PreAdd()
        {
            u32 count = GetCount();
            if (count >= m_Container.GetAllocatedCount())
                m_Container.Resize(count + 1, false);
            else
                m_Container.IncrementCount();
            return count;
        }

    private:
        Container<T> m_Container;
    };
}