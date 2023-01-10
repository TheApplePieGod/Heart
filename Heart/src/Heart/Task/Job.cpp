#include "hepch.h"
#include "Task.h"

#include "Heart/Task/JobManager.h"

namespace Heart
{
    Job::Job(const Job& other)
    {
        Copy(other);
    }

    Job::Job(u32 handle)
        : m_Handle(handle)
    {
        if (m_Handle == InvalidHandle) return;
        JobManager::IncrementRefCount(m_Handle);
    }

    Job::~Job()
    {
        if (m_Handle == InvalidHandle) return;
        JobManager::DecrementRefCount(m_Handle, true);
    }

    bool Job::Wait(u32 timeout) const
    {
        return JobManager::Wait(*this, timeout);
    }

    void Job::Copy(const Job& other)
    {
        if (m_Handle != InvalidHandle)
            JobManager::DecrementRefCount(m_Handle, true);
        m_Handle = other.m_Handle;
        if (m_Handle == InvalidHandle) return;
        JobManager::IncrementRefCount(m_Handle);
    }
}
