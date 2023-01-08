#pragma once

namespace Heart
{
    class Task
    {
    public:
        Task();
        Task(const Task& other);
        Task(u32 handle);
        ~Task();
        
        bool Wait(u32 timeout = 0);
        
        inline u32 GetHandle() const { return m_Handle; }
        
        inline void operator=(const Task& other) { Copy(other); }
        
    private:
        void Copy(const Task& other);
        
    private:
        u32 m_Handle;
    };
}
