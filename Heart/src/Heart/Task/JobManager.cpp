#include "hepch.h"
#include "JobManager.h"

#include "Heart/Core/Timing.h"

namespace Heart
{
    void JobManager::Initialize(u32 numWorkers)
    {
        if (numWorkers == 0)
        {
            s_SingleThreaded = true;
            s_Initialized = true;

            return;
        }

        // Populate initial data to prevent resizing
        s_JobList.Resize(5000);
        s_HandleFreeList.Reserve(s_JobList.Count());
        for (u32 i = 0; i < s_JobList.Count(); i++)
            s_HandleFreeList.Add(i);
            
        s_Initialized = true;

        s_ExecuteQueues.Resize(numWorkers);
        for (u32 i = 0; i < numWorkers; i++)
            s_WorkerThreads.AddInPlace(&JobManager::ProcessQueue, i);
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

        HVector<size_t> indices;
        for (size_t i = 0; i < count; i++)
        {
            if (!check(i)) continue;
            indices.AddInPlace(i);
        }

        u32 handle = ScheduleInternal(indices, std::move(job), std::move(check));
        return Job(handle, false);
    }

    u32 JobManager::ScheduleInternal(const HVector<size_t>& indices, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check)
    {
        if (s_SingleThreaded)
        {
            for (size_t i = 0; i < indices.Count(); i++)
                job(i);
            
            return 0;
        }

        u32 handle = CreateJob();

        JobData& data = s_JobList[handle];
        data.Mutex.lock();
        data.Complete = indices.Count() == 0;
        data.Remaining = indices.Count();
        data.RefCount = indices.Count() == 0 ? 1 : 2;
        data.Job = std::move(job);
        data.Mutex.unlock();

        if (indices.IsEmpty())
            return handle;

        u32 countPerThread = indices.Count() / s_ExecuteQueues.Count();
        for (u32 i = 0; i < s_ExecuteQueues.Count(); i++)
        {
            auto& queue = s_ExecuteQueues[i];
            queue.Mutex.lock();
            auto& entry = queue.Queue.emplace();
            entry.Handle = handle;
            entry.Indices.CopyFrom(
                indices.begin() + i * countPerThread,
                i == s_ExecuteQueues.Count() - 1
                    ? indices.end()
                    : indices.begin() + (i + 1) * countPerThread
            );
            queue.Mutex.unlock();
            queue.QueueCV.notify_all();
        }

        return handle;
    }

    bool JobManager::Wait(const Job& job, u32 timeout)
    {
        if (job.GetHandle() == Job::InvalidHandle) return false;
        if (s_SingleThreaded) return true;

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
        static void ScheduleInternal(std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check);

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
        if (!s_Initialized || s_SingleThreaded) return;

        s_JobList[handle].RefCount++;
    }

    void JobManager::DecrementRefCount(u32 handle)
    {
        if (!s_Initialized || s_SingleThreaded) return;

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
            
            auto executeData = std::move(queue.Queue.front());
            queue.Queue.pop();
            lock.unlock();
            
            auto& data = s_JobList[executeData.Handle];

            // Lock and unlock the mutex to ensure job data has been written
            // TODO: this is technically not good enough because in theory this lock could get scheduled
            // first by the OS. However, this seems to be good enough for now because there is still
            // a decent amount of delay between this lock and the initial lock
            data.Mutex.lock();
            data.Mutex.unlock();

            for (size_t val : executeData.Indices)
                data.Job(val);
                
            // Decrement job completion count
            if (data.Remaining.fetch_sub(executeData.Indices.Count()) == executeData.Indices.Count())
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
