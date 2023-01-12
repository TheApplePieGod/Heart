#include "hepch.h"
#include "Task.h"

#include "Heart/Task/TaskManager.h"

namespace Heart
{
    Task::Task(const Task& other)
    {
        Copy(other);
    }

    Task::Task(u32 handle, bool incref)
        : m_Handle(handle)
    {
        if (m_Handle == InvalidHandle) return;
        if (incref)
            TaskManager::IncrementRefCount(m_Handle);
    }

    Task::~Task()
    {
        if (m_Handle == InvalidHandle) return;
        TaskManager::DecrementRefCount(m_Handle);
    }

    bool Task::Wait(u32 timeout) const
    {
        return TaskManager::Wait(*this, timeout);
    }

    void Task::Copy(const Task& other)
    {
        if (m_Handle != InvalidHandle)
            TaskManager::DecrementRefCount(m_Handle);
        m_Handle = other.m_Handle;
        if (m_Handle == InvalidHandle) return;
        TaskManager::IncrementRefCount(m_Handle);
    }

    TaskGroup::TaskGroup(std::initializer_list<Task> tasks)
        : m_Tasks(tasks)
    {}

    bool TaskGroup::Wait() const
    {
        for (const Task& task : m_Tasks)
            task.Wait();
        return true;
    }

    bool TaskGroup::Wait(u32 timeout) const
    {
        auto start = std::chrono::system_clock::now();
        for (const Task& task : m_Tasks)
        {
            if (!task.Wait(timeout))
                return false;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() >= timeout)
                return false;
        }
        return true;
    }
}
