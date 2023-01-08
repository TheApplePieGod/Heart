#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Core/Task.h"

namespace Heart
{
    class TaskManager
    {
    public:
        static void Initialize(u32 numWorkers);
        static void Shutdown();
        
        static Task Schedule(std::function<void()>&& task, Task dependency = Task());
        static Task Schedule(std::function<void()>&& task, std::initializer_list<Task> dependencies);
        static Task Schedule(std::function<void()>&& task, const HVector<Task>& dependencies);
        
        static bool Wait(const Task& task, u32 timeout); // milliseconds
        
        inline static constexpr u32 InvalidHandle = std::numeric_limits<u32>::max();
        
    private:
        struct TaskData
        {
            bool Complete = false;
            bool Success = false;
            bool ShouldExecute = true;
            HVector<u32> Dependents;
            std::function<void()> Task = nullptr;
            std::atomic<u32> DependencyCount = 0;
            std::atomic<u32> RefCount = 0;
            std::condition_variable CompletionCV;
            std::mutex Mutex;
            HString8 Name;
        };

    private:
        static void PushHandleToQueue(u32 handle);
        static void IncrementRefCount(u32 handle);
        static void DecrementRefCount(u32 handle, bool lock);
        static void ProcessQueue();
        
    private:
        inline static std::queue<u32> s_ExecuteQueue;
        inline static HVector<u32> s_HandleFreeList;
        inline static HVector<TaskData> s_TaskList;
        inline static HVector<std::thread> s_WorkerThreads;
        
        inline static std::shared_mutex s_TaskListMutex;
        inline static std::mutex s_FreeListMutex;
        inline static std::mutex s_ExecuteQueueMutex;
        inline static std::condition_variable s_ExecuteQueueCV;
        
        inline static bool s_Initialized = false;
        
        friend class Task;
    };
}
