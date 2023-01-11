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
        {
            s_ExecuteQueues.AddInPlace();
            s_WorkerThreads.AddInPlace(&JobManager::ProcessQueue, i);
        }
    }

    void JobManager::Shutdown()
    {
        s_Initialized = false;
        
        // Wake all worker threads
        for (auto& queue : s_ExecuteQueues)
            queue.QueueCV.notify_all();
        
        // Now wait for completion
        for (auto& thread : s_WorkerThreads)
            thread.join();
    }

    Job JobManager::Schedule(size_t count, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check)
    {
        u32 handle = CreateJob();
        
        std::uniform_int_distribution<int> distribution(0, (int)s_ExecuteQueues.Count() - 1);
        
        for (auto& queue : s_ExecuteQueues)
            queue.Mutex.lock();
        
        size_t realCount = 0;
        for (size_t i = 0; i < count; i++)
        {
            if (!check(i)) continue;
            s_ExecuteQueues[distribution(s_Generator)].Queue.emplace(handle, i);
            realCount++;
        }

        s_JobListMutex.lock_shared();
        JobData& data = s_JobList[handle];
        data.Complete = realCount == 0;
        data.Remaining = realCount;
        data.RefCount = realCount == 0 ? 0 : 1;
        data.Job = std::move(job);
        s_JobListMutex.unlock_shared();

        for (auto& queue : s_ExecuteQueues)
        {
            queue.Mutex.unlock();
            queue.QueueCV.notify_all();
        }
        
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
                [&data]{ return data.Complete; }
            );
        }
        else
            data.CompletionCV.wait(lock, [&data]{ return data.Complete; });
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

    void JobManager::IncrementRefCount(u32 handle)
    {
        s_JobListMutex.lock_shared();
        s_JobList[handle].RefCount++;
        s_JobListMutex.unlock_shared();
    }

    void JobManager::DecrementRefCount(u32 handle, bool lock)
    {
        if (!s_Initialized) return;

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

    void JobManager::ProcessQueue(u32 workerIndex)
    {
        const auto pred = [workerIndex]{ return !s_ExecuteQueues[workerIndex].Queue.empty() || !s_Initialized; };

        auto& queue = s_ExecuteQueues[workerIndex];
        std::unique_lock lock(queue.Mutex);
        while (s_Initialized)
        {
            queue.QueueCV.wait(lock, pred);
            
            if (!s_Initialized) break;
            
            // lock already locked by wait()
            auto executeData = std::move(queue.Queue.front());
            queue.Queue.pop();
            lock.unlock();
            
            s_JobListMutex.lock_shared();
            auto& data = s_JobList[executeData.Handle];
            data.Job(executeData.Index);
                
            // Decrement job completion count
            if (--data.Remaining == 0)
            {
                data.Mutex.lock();
                data.Complete = true;
                data.Mutex.unlock();
                data.CompletionCV.notify_all();
                DecrementRefCount(executeData.Handle, false);
            }
            s_JobListMutex.unlock_shared();
            
            // Relock lock before next iteration (required by wait())
            lock.lock();
        }
    }
}
