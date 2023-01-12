#pragma once

#include "Heart/Container/HVector.hpp"

namespace Heart
{
    class Job
    {
    public:
        Job() = default;
        Job(const Job& other);
        Job(u32 handle, bool incref = true);
        ~Job();

        bool Wait(u32 timeout = 0) const;
        
        inline u32 GetHandle() const { return m_Handle; }
        
        inline void operator=(const Job& other) { Copy(other); }
        
        inline static constexpr u32 InvalidHandle = std::numeric_limits<u32>::max();

    private:
        void Copy(const Job& other);
        
    private:
        u32 m_Handle = InvalidHandle;
    };

}
