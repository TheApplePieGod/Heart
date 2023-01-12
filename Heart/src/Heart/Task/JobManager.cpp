#include "hepch.h"
#include "JobManager.h"

#include "Heart/Core/Timing.h"

namespace Heart
{
    void JobManager::Initialize(u32 numWorkers)
    {
        // Populate initial data to prevent resizing
        s_JobList.Resize(5000);
        s_HandleFreeList.Reserve(s_JobList.Count());
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
        HE_PROFILE_FUNCTION();

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

        JobData& data = s_JobList[handle];
        data.Mutex.lock();
        data.Complete = realCount == 0;
        data.Remaining = realCount;
        data.RefCount = realCount == 0 ? 1 : 2;
        data.Job = std::move(job);
        data.Mutex.unlock();

        for (auto& queue : s_ExecuteQueues)
        {
            queue.Mutex.unlock();
            queue.QueueCV.notify_all();
        }
        
        return Job(handle, false);
    }

    bool JobManager::Wait(const Job& job, u32 timeout)
    {
        if (job.GetHandle() == Job::InvalidHandle) return false;

        HE_PROFILE_FUNCTION();
        
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
        
        return complete;
    }

    u32 JobManager::CreateJob()
    {
        s_FreeListMutex.lock();
        HE_ENGINE_ASSERT(s_HandleFreeList.Count() != 0, "Scheduled too many jobs!");
        u32 handle = s_HandleFreeList.Back();
        s_HandleFreeList.Pop();
        s_FreeListMutex.unlock();
        
        return handle;
    }

    void JobManager::IncrementRefCount(u32 handle)
    {
        s_JobList[handle].RefCount++;
    }

    void JobManager::DecrementRefCount(u32 handle)
    {
        if (!s_Initialized) return;

        auto& data = s_JobList[handle];
        if (--data.RefCount == 0)
        {
            // Clear func to potentially free resources
            data.Mutex.lock();
            data.Job = nullptr;
            data.Mutex.unlock();

            s_FreeListMutex.lock();
            s_HandleFreeList.Add(handle);
            s_FreeListMutex.unlock();
        }
    }

    void JobManager::ProcessQueue(u32 workerIndex)
    {
        HE_PROFILE_THREAD("Job Thread");

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
            
            auto& data = s_JobList[executeData.Handle];
            data.Job(executeData.Index);
                
            // Decrement job completion count
            if (--data.Remaining == 0)
            {
                data.Mutex.lock();
                data.Complete = true;
                data.Mutex.unlock();
                DecrementRefCount(executeData.Handle);
                data.CompletionCV.notify_all();
            }
            
            // Relock lock before next iteration (required by wait())
            lock.lock();
        }
    }
}
