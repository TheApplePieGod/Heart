#include "hepch.h"
#include "JobManager.h"

#include "Heart/Core/Timing.h"

namespace Heart
{
    void JobManager::Initialize(u32 numWorkers)
    {
        // Populate initial data to prevent resizing
        s_JobList.Resize(5000);
        s_HandleFreeList.Reserve(5000);
        for (u32 i = 0; i < s_JobList.Count(); i++)
            s_HandleFreeList.Add(i);
            
        s_Initialized = true;
        
        for (u32 i = 0; i < numWorkers; i++)
            s_WorkerThreads.AddInPlace(&JobManager::ProcessQueue);
    }

    void JobManager::Shutdown()
    {
        s_Initialized = false;
        
        // Wake all worker threads
        s_ExecuteQueueCV.notify_all();
        
        // Now wait for completion
        for (auto& thread : s_WorkerThreads)
            thread.join();
    }

    Job JobManager::Schedule(std::function<void(size_t)>&& job, size_t count)
    {
        u32 handle = CreateJob();
        
        s_JobListMutex.lock_shared();
        JobData& data = s_JobList[handle];
        data.Remaining = count;
        data.RefCount = 1;
        data.Job = std::move(job);
        data.Indices.Resize(count);
        for (size_t i = 0; i < count; i++)
            data.Indices[i] = i;
        s_JobListMutex.unlock_shared();
        
        PushHandleToQueue(handle);
        
        return Job(handle);
    }

    bool JobManager::Wait(const Job& job, u32 timeout)
    {
        if (job.GetHandle() == Job::InvalidHandle) return false;
        
        s_JobListMutex.lock_shared();
        auto& data = s_JobList[job.GetHandle()];
        std::unique_lock<std::mutex> lock(data.Mutex);
        bool complete = true;
        if (timeout)
        {
            complete = data.CompletionCV.wait_for(
                lock,
                std::chrono::milliseconds(timeout),
                [&data]{ return data.Remaining == 0; }
            );
        }
        else
            data.CompletionCV.wait(lock, [&data]{ return data.Remaining == 0; });
        s_JobListMutex.unlock_shared();
        
        return complete;
    }

    u32 JobManager::CreateJob()
    {
        u32 handle;
        s_FreeListMutex.lock();
        if (s_HandleFreeList.Count() == 0)
        {
            s_FreeListMutex.unlock();
            handle = s_JobList.Count();
            s_JobListMutex.lock();
            s_JobList.AddInPlace();
            s_JobListMutex.unlock();
        }
        else
        {
            handle = s_HandleFreeList.Back();
            s_HandleFreeList.Pop();
            s_FreeListMutex.unlock();
        }
        
        return handle;
    }

    void JobManager::PushHandleToQueue(u32 handle)
    {
        s_ExecuteQueueMutex.lock();
        s_ExecuteQueue.push(handle);
        s_ExecuteQueueMutex.unlock();
        s_ExecuteQueueCV.notify_all();
    }

    void JobManager::IncrementRefCount(u32 handle)
    {
        s_JobListMutex.lock_shared();
        s_JobList[handle].RefCount++;
        s_JobListMutex.unlock_shared();
    }

    void JobManager::DecrementRefCount(u32 handle, bool lock)
    {
        if (lock)
            s_JobListMutex.lock_shared();
        auto& data = s_JobList[handle];
        if (--data.RefCount == 0)
        {
            s_FreeListMutex.lock();
            s_HandleFreeList.Add(handle);
            s_FreeListMutex.unlock();
        }
        if (lock)
            s_JobListMutex.unlock_shared();
    }

    void JobManager::ProcessQueue()
    {
        std::unique_lock lock(s_ExecuteQueueMutex);
        while (s_Initialized)
        {
            s_ExecuteQueueCV.wait(lock, []{ return !s_ExecuteQueue.empty() || !s_Initialized; });
            
            if (!s_Initialized) break;
            
            // lock already locked by wait()
            u32 handle = s_ExecuteQueue.front();
            lock.unlock();
            
            s_JobListMutex.lock_shared();
            auto& data = s_JobList[handle]
            size_t remaining = data.Remaining--;
            while (remaining > 0)
            {
                execution.Func(execution.Index);
                
            }
                
            // Decrement job completion count
            auto& data = s_JobList[execution.Handle];
            if (--data.Remaining == 0)
            {
                data.CompletionCV.notify_all();
                DecrementRefCount(execution.Handle, false);
            }
            s_JobListMutex.unlock_shared();
            
            // Relock lock before next iteration (required by wait())
            lock.lock();
        }
    }
}
