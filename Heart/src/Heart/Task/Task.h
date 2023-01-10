#pragma once

#include "Heart/Container/HVector.hpp"

namespace Heart
{
    class Task
    {
    public:
        Task() = default;
        Task(const Task& other);
        Task(u32 handle);
        ~Task();
        
        bool Wait(u32 timeout = 0) const;
        
        inline u32 GetHandle() const { return m_Handle; }
        
        inline void operator=(const Task& other) { Copy(other); }
        
        inline static constexpr u32 InvalidHandle = std::numeric_limits<u32>::max();
        
    private:
        void Copy(const Task& other);
        
    private:
        u32 m_Handle = InvalidHandle;
    };

    class TaskGroup
    {
    public:
        TaskGroup() = default;
        TaskGroup(std::initializer_list<Task> tasks);
        
        inline void AddTask(const Task& task) { m_Tasks.AddInPlace(task); }
        
        bool Wait() const;
        bool Wait(u32 timeout) const;
        
        inline const auto& GetTasks() const { return m_Tasks; }
        
    private:
        HVector<Task> m_Tasks;
    };
}
