#include "hepch.h"
#include "TaskManager.h"

namespace Heart
{
    void TaskManager::Initialize(u32 numWorkers)
    {
        // Populate initial data to prevent resizing
        s_TaskList.Resize(500);
        s_HandleFreeList.Reserve(500);
        for (u32 i = 0; i < 500; i++)
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

    Task TaskManager::Schedule(std::function<void()>&& task, Task dependency)
    {
        u32 handle;
        s_FreeListMutex.lock();
        if (s_HandleFreeList.Count() == 0)
        {
            s_FreeListMutex.unlock();
            handle = s_TaskList.Count();
            s_TaskListMutex.lock();
            s_TaskList.AddInPlace();
            s_TaskListMutex.unlock();
        }
        else
        {
            handle = s_HandleFreeList.Back();
            s_HandleFreeList.Pop();
            s_FreeListMutex.unlock();
        }
        
        s_TaskListMutex.lock_shared();
        TaskData& data = s_TaskList[handle];
        data.Complete = false;
        data.Success = false;
        data.ShouldExecute = true;
        data.Dependents.Clear();
        data.Task = task;
        data.DependencyCount = dependency.GetHandle() == InvalidHandle ? 0 : 1;
        data.Name = "";
        // Set the initial refcount to one because we'll consider a task before it is completed as having a reference to
        // itself. This saves some complexity when decrementing since we no longer need to check for completion
        data.RefCount = 1;
        s_TaskListMutex.unlock_shared();
        
        bool executeNow = false;
        // No dependencies so it can be executed whenever
        if (dependency.GetHandle() == InvalidHandle)
            executeNow = true;
        // Otherwise we must add this to the dependents list of the dependencies unless it is already
        // completed
        else
        {
            s_TaskListMutex.lock_shared();
            TaskData& dependencyData = s_TaskList[dependency.GetHandle()];
            dependencyData.Mutex.lock();
            if (dependencyData.Complete)
                executeNow = true;
            else
                dependencyData.Dependents.Add(handle);
            dependencyData.Mutex.unlock();
            s_TaskListMutex.unlock_shared();
        }
        
        if (executeNow)
            PushHandleToQueue(handle);
        
        return Task(handle);
    }

    bool TaskManager::Wait(const Task& task, u32 timeout)
    {
        if (task.GetHandle() == InvalidHandle) return false;
        
        s_TaskListMutex.lock_shared();
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
        s_TaskListMutex.unlock_shared();
        
        return complete;
    }

    void TaskManager::PushHandleToQueue(u32 handle)
    {
        s_ExecuteQueueMutex.lock();
        s_ExecuteQueue.push(handle);
        s_ExecuteQueueMutex.unlock();
        s_ExecuteQueueCV.notify_one();
    }

    void TaskManager::IncrementRefCount(u32 handle)
    {
        s_TaskListMutex.lock_shared();
        s_TaskList[handle].RefCount++;
        s_TaskListMutex.unlock_shared();
    }

    void TaskManager::DecrementRefCount(u32 handle, bool lock)
    {
        if (lock)
            s_TaskListMutex.lock_shared();
        auto& data = s_TaskList[handle];
        if (--data.RefCount == 0)
        {
            // Clear func to potentially free resources
            data.Task = nullptr;
            
            HE_ENGINE_LOG_WARN("HANDLE FREE");
            
            s_FreeListMutex.lock();
            s_HandleFreeList.Add(handle);
            s_FreeListMutex.unlock();
        }
        if (lock)
            s_TaskListMutex.unlock_shared();
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
            s_ExecuteQueue.pop();
            lock.unlock();
            
            // Retrieve func to execute
            s_TaskListMutex.lock_shared();
            auto func = s_TaskList[handle].Task;
            s_TaskListMutex.unlock_shared();
            
            try
            {
                func();
                
                // Set success flag
                s_TaskListMutex.lock_shared();
                s_TaskList[handle].Success = true;
            }
            catch (const std::exception& e)
            {
                s_TaskListMutex.lock_shared();
                HE_ENGINE_LOG_WARN("Task '{0}' failed with an exception: {1}", s_TaskList[handle].Name.Data(), e.what());
            }
            
            // Mark complete and update all dependents
            auto& data = s_TaskList[handle];
            data.Mutex.lock();
            data.Complete = true;
            for (u32 dep : data.Dependents)
            {
                auto& depData = s_TaskList[dep];
                if (--depData.DependencyCount == 0)
                    PushHandleToQueue(dep);
            }
            DecrementRefCount(handle, false);
            data.Mutex.unlock();
            data.CompletionCV.notify_all();
            
            s_TaskListMutex.unlock_shared();
            
            // Relock lock before next iteration (required by wait())
            lock.lock();
        }
    }
}
