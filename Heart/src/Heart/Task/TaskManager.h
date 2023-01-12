#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString8.h"
#include "Heart/Task/Task.h"

namespace Heart
{
    class TaskManager
    {
    public:
        static void Initialize(u32 numWorkers);
        static void Shutdown();
        
        template <class Iter, class Func>
        static TaskGroup ScheduleIter(Func task, Iter begin, Iter end, const TaskGroup& dependencies)
        {
            TaskGroup group;
            for (; begin != end; ++begin)
                group.AddTask(Schedule([begin, &task](){ task(*begin); }, dependencies));
            return group;
        }
        
        static Task Schedule(std::function<void()>&& task, HStringView8 name);
        static Task Schedule(std::function<void()>&& task, const Task& dependency);
        static Task Schedule(std::function<void()>&& task, const TaskGroup& dependencies);
        static Task Schedule(std::function<void()>&& task, std::initializer_list<Task> dependencies);
        static Task Schedule(std::function<void()>&& task, const HVector<Task>& dependencies);
        static Task Schedule(std::function<void()>&& task, const Task* dependencies, u32 dependencyCount, HStringView8 name);
        
        static bool Wait(const Task& task, u32 timeout); // milliseconds
        
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
        static void DecrementRefCount(u32 handle);
        static void ProcessQueue();
        
    private:
        inline static std::queue<u32> s_ExecuteQueue;
        inline static HVector<u32> s_HandleFreeList;
        inline static HVector<TaskData> s_TaskList;
        inline static HVector<std::thread> s_WorkerThreads;
        
        inline static std::mutex s_FreeListMutex;
        inline static std::mutex s_ExecuteQueueMutex;
        inline static std::condition_variable s_ExecuteQueueCV;
        
        inline static bool s_Initialized = false;
        
        friend class Task;
    };
}
