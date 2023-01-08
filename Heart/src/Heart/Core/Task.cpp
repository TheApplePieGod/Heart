#include "hepch.h"
#include "Task.h"

#include "Heart/Core/TaskManager.h"

namespace Heart
{
    Task::Task()
        : m_Handle(TaskManager::InvalidHandle)
    {}

    Task::Task(const Task& other)
    {
        Copy(other);
    }

    Task::Task(u32 handle)
        : m_Handle(handle)
    {
        if (m_Handle == TaskManager::InvalidHandle) return;
        TaskManager::IncrementRefCount(m_Handle);
    }

    Task::~Task()
    {
        if (m_Handle == TaskManager::InvalidHandle) return;
        TaskManager::DecrementRefCount(m_Handle, true);
    }

    bool Task::Wait(u32 timeout)
    {
        return TaskManager::Wait(*this, timeout);
    }

    void Task::Copy(const Task& other)
    {
        if (m_Handle != TaskManager::InvalidHandle)
            TaskManager::DecrementRefCount(m_Handle, true);
        m_Handle = other.m_Handle;
        if (m_Handle == TaskManager::InvalidHandle) return;
        TaskManager::IncrementRefCount(m_Handle);
    }
}
