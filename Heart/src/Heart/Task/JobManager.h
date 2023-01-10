#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Task/Job.h"

namespace Heart
{
    class JobManager
    {
    public:
        static void Initialize(u32 numWorkers);
        static void Shutdown();
        
        static Job Schedule(std::function<void(size_t)>&& job, size_t count);
        
        template<typename Iter>
        static Job ScheduleIter(std::function<void(size_t)>&& job, Iter begin, Iter end);
        
        static bool Wait(const Job& job, u32 timeout); // milliseconds
    
    private:
        struct JobData
        {
            HVector<size_t> Indices;
            std::function<void(size_t)> Job = nullptr;
            std::atomic<u32> RefCount;
            std::atomic<size_t> Remaining;
            std::mutex Mutex;
            std::condition_variable CompletionCV;
        };
        
    private:
        static u32 CreateJob();
        static void PushHandleToQueue(u32 handle);
        static void IncrementRefCount(u32 handle);
        static void DecrementRefCount(u32 handle, bool lock);
        static void ProcessQueue();
        
    private:
        inline static std::queue<u32> s_ExecuteQueue;
        inline static HVector<u32> s_HandleFreeList;
        inline static HVector<JobData> s_JobList;
        inline static HVector<std::thread> s_WorkerThreads;
        
        inline static std::shared_mutex s_JobListMutex;
        inline static std::mutex s_FreeListMutex;
        inline static std::mutex s_ExecuteQueueMutex;
        inline static std::condition_variable s_ExecuteQueueCV;
        
        inline static bool s_Initialized = false;
        
        friend class Job;
    };

    template<typename Iter>
    Job JobManager::ScheduleIter(std::function<void(size_t)>&& job, Iter begin, Iter end)
    {
        u32 handle = CreateJob();
        
        s_ExecuteQueueMutex.lock();
        size_t count = 0;
        for (; begin != end; ++begin)
        {
            s_ExecuteQueue.emplace(std::move(job), (size_t)*begin, handle);
            count++;
        }
        
        InitJob(handle, count);
        
        s_ExecuteQueueMutex.unlock();
        s_ExecuteQueueCV.notify_all();
        
        return Job(handle);
    }

}
