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
        
        static Job Schedule(size_t count, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check = [](size_t index){ return true; });
        
        template<typename Iter>
        static Job ScheduleIter(Iter begin, Iter end, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check = [](size_t index){ return true; });
        
        static bool Wait(const Job& job, u32 timeout); // milliseconds
    
    private:
        struct JobData
        {
            bool Complete;
            std::function<void(size_t)> Job = nullptr;
            std::atomic<u32> RefCount;
            std::atomic<u32> Remaining;
            std::mutex Mutex;
            std::condition_variable CompletionCV;
        };
        
        struct ExecutionData
        {
            u32 Handle;
            HVector<size_t> Indices;
        };
        
        struct WorkerQueue
        {
            std::queue<ExecutionData> Queue;
            std::mutex Mutex;
            std::condition_variable QueueCV;
        };
        
    private:
        static u32 ScheduleInternal(const HVector<size_t>& indices, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check);
        static u32 CreateJob();
        static void IncrementRefCount(u32 handle);
        static void DecrementRefCount(u32 handle);
        static void ProcessQueue(u32 workerIndex);
        
    private:
        inline static HVector<WorkerQueue> s_ExecuteQueues;
        inline static HVector<u32> s_HandleFreeList;
        inline static HVector<JobData> s_JobList;
        inline static HVector<std::thread> s_WorkerThreads;
        
        inline static std::mutex s_FreeListMutex;
        
        inline static bool s_Initialized = false;
        inline static bool s_SingleThreaded = false;
        
        friend class Job;
    };

    template<typename Iter>
    Job JobManager::ScheduleIter(Iter begin, Iter end, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check)
    {
        HE_PROFILE_FUNCTION();

        HVector<size_t> indices;
        for (; begin != end; ++begin)
        {
            size_t idx = (size_t)*begin;
            if (!check(idx)) continue;
            indices.AddInPlace((size_t)*begin);
        }

        u32 handle = ScheduleInternal(indices, std::move(job), std::move(check));
        return Job(handle, false);
    }

}
