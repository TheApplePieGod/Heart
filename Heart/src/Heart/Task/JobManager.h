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
            std::atomic<size_t> Remaining;
            std::mutex Mutex;
            std::condition_variable CompletionCV;
        };
        
        struct ExecutionData
        {
            ExecutionData(u32 handle, size_t index)
                : Handle(handle), Index(index)
            {}

            u32 Handle;
            size_t Index;
        };
        
        struct WorkerQueue
        {
            std::queue<ExecutionData> Queue;
            std::mutex Mutex;
            std::condition_variable QueueCV;
        };
        
    private:
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
        inline static std::mt19937 s_Generator;
        
        inline static bool s_Initialized = false;
        
        friend class Job;
    };

    template<typename Iter>
    Job JobManager::ScheduleIter(Iter begin, Iter end, std::function<void(size_t)>&& job, std::function<bool(size_t)>&& check)
    {
        HE_PROFILE_FUNCTION();

        u32 handle = CreateJob();
        
        for (auto& queue : s_ExecuteQueues)
            queue.Mutex.lock();
        
        size_t count = 0;
        std::uniform_int_distribution<int> distribution(0, (int)s_ExecuteQueues.Count() - 1);
        for (; begin != end; ++begin)
        {
            size_t idx = (size_t)*begin;
            if (!check(idx)) continue;
            s_ExecuteQueues[distribution(s_Generator)].Queue.emplace(handle, (size_t)*begin);
            count++;
        }

        JobData& data = s_JobList[handle];
        data.Mutex.lock();
        data.Complete = count == 0;
        data.Remaining = count;
        data.RefCount = count == 0 ? 1 : 2;
        data.Job = std::move(job);
        data.Mutex.unlock();

        for (auto& queue : s_ExecuteQueues)
        {
            queue.Mutex.unlock();
            queue.QueueCV.notify_all();
        }
        
        return Job(handle, false);
    }

}
