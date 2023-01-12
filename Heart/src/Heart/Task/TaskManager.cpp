#include "hepch.h"
#include "TaskManager.h"

namespace Heart
{
    void TaskManager::Initialize(u32 numWorkers)
    {
        // Populate initial data to prevent resizing
        s_TaskList.Resize(5000);
        s_HandleFreeList.Reserve(s_TaskList.Count());
        for (u32 i = 0; i < s_TaskList.Count(); i++)
            s_HandleFreeList.Add(i);
            
        s_Initialized = true;
        
        for (u32 i = 0; i < numWorkers; i++)
            s_WorkerThreads.AddInPlace(&TaskManager::ProcessQueue);
    }

    void TaskManager::Shutdown()
    {
        s_Initialized = false;
        
        // Wake all worker threads
        s_ExecuteQueueCV.notify_all();
        
        // Now wait for completion
        for (auto& thread : s_WorkerThreads)
            thread.join();
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, HStringView8 name)
    {
        return Schedule(std::move(task), priority, nullptr, 0, name);
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, const Task& dependency)
    {
        return Schedule(std::move(task), priority, &dependency, 1, "");
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, const TaskGroup& dependencies)
    {
        return Schedule(std::move(task), priority, dependencies.GetTasks().Data(), dependencies.GetTasks().Count(), "");
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, std::initializer_list<Task> dependencies)
    {
        return Schedule(std::move(task), priority, dependencies.begin(), dependencies.size(), "");
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, const HVector<Task>& dependencies)
    {
        return Schedule(std::move(task), priority, dependencies.Data(), dependencies.Count(), "");
    }

    Task TaskManager::Schedule(std::function<void()>&& task, Task::Priority priority, const Task* dependencies, u32 dependencyCount, HStringView8 name)
    {
        s_FreeListMutex.lock();
        HE_ENGINE_ASSERT(s_HandleFreeList.Count() != 0, "Scheduled too many tasks!");
        u32 handle = s_HandleFreeList.Back();
        s_HandleFreeList.Pop();
        s_FreeListMutex.unlock();
        
        TaskData& data = s_TaskList[handle];
        data.Mutex.lock();
        data.Complete = false;
        data.Success = false;
        data.ShouldExecute = true;
        data.Dependents.Clear();
        data.Task = std::move(task);
        data.DependencyCount = dependencyCount;
        data.Name = name;
        data.Priority = priority;
        // Increase the initial refcount by one because we'll consider a task before it is completed as having a reference to
        // itself. This saves some complexity when decrementing since we no longer need to check for completion. Increase it
        // by an additional one because we want to ensure the task doesn't get completed and the refcount go to zero before
        // the task object gets constructed because that would cause the refcount to go to zero twice
        data.RefCount = 2;
        data.Mutex.unlock();
        
        bool executeNow = false;
        // No dependencies so it can be executed whenever
        if (dependencyCount == 0)
            executeNow = true;
        // Otherwise we must add this to the dependents list of the dependencies unless it is already
        // completed
        else
        {
            for (u32 i = 0; i < dependencyCount; i++)
            {
                if (dependencies[i].GetHandle() == Task::InvalidHandle) continue;
                TaskData& dependencyData = s_TaskList[dependencies[i].GetHandle()];
                dependencyData.Mutex.lock();
                if (dependencyData.Complete)
                    executeNow = true;
                else
                    dependencyData.Dependents.Add(handle);
                dependencyData.Mutex.unlock();
            }
        }
        
        if (executeNow)
            PushHandleToQueue(handle);
        
        return Task(handle, false);
    }

    bool TaskManager::Wait(const Task& task, u32 timeout)
    {
        if (task.GetHandle() == Task::InvalidHandle) return false;
        
        auto& data = s_TaskList[task.GetHandle()];
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

    void TaskManager::PushHandleToQueue(u32 handle)
    {
        auto& data = s_TaskList[handle];
        s_ExecuteQueueMutex.lock();
        switch (data.Priority)
        {
            default:
            case Task::Priority::High:
            { s_ExecuteQueue.push_front(handle); } break;
            case Task::Priority::Medium:
            { s_ExecuteQueue.insert(s_ExecuteQueue.begin() + s_ExecuteQueue.size() / 2, handle); } break;
            case Task::Priority::Low:
            { s_ExecuteQueue.push_back(handle); } break;
        }
        s_ExecuteQueueMutex.unlock();
        s_ExecuteQueueCV.notify_one();
    }

    void TaskManager::IncrementRefCount(u32 handle)
    {
        s_TaskList[handle].RefCount++;
    }

    void TaskManager::DecrementRefCount(u32 handle)
    {
        if (!s_Initialized) return;

        auto& data = s_TaskList[handle];
        if (--data.RefCount == 0)
        {
            // Clear func to potentially free resources
            data.Task = nullptr;
            
            s_FreeListMutex.lock();
            s_HandleFreeList.Add(handle);
            s_FreeListMutex.unlock();
        }
    }

    // TODO: priority

    void TaskManager::ProcessQueue()
    {
        std::unique_lock lock(s_ExecuteQueueMutex);
        while (s_Initialized)
        {
            s_ExecuteQueueCV.wait(lock, []{ return !s_ExecuteQueue.empty() || !s_Initialized; });
            
            if (!s_Initialized) break;
            
            // lock already locked by wait()
            u32 handle = s_ExecuteQueue.front();
            s_ExecuteQueue.pop_front();
            lock.unlock();
            
            auto& data = s_TaskList[handle];
            data.Mutex.lock();
            try
            {
                HE_ENGINE_ASSERT(data.Task);
                data.Task();
                
                // Set success flag
                data.Success = true;
            }
            catch (const std::exception& e)
            {
                HE_ENGINE_LOG_WARN("Task '{0}' failed with an exception: {1}", s_TaskList[handle].Name.Data(), e.what());
            }
            
            // Mark complete and update all dependents
            data.Complete = true;
            for (u32 dep : data.Dependents)
            {
                auto& depData = s_TaskList[dep];
                if (--depData.DependencyCount == 0)
                    PushHandleToQueue(dep);
            }
            DecrementRefCount(handle);
            data.Mutex.unlock();
            data.CompletionCV.notify_all();
            
            // Relock lock before next iteration (required by wait())
            lock.lock();
        }
    }
}
