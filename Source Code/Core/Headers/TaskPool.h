/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _TASK_POOL_H_
#define _TASK_POOL_H_

#include "Platform/Threading/Headers/Task.h"

namespace Divide {

class TaskPool {
  protected:
    enum class TaskState : U8 {
        TASK_FREE = 0,
        TASK_ALLOCATED,
        TASK_RUNNING
    };

  public:
    enum class TaskPoolType : U8 {
        PRIORITY_QUEUE = 0,
        FIFO_QUEUE,
        DONT_CARE,
        COUNT
    };

    explicit TaskPool(U32 maxTaskCount);
    ~TaskPool();
    
    bool init(U32 threadCount, TaskPoolType type, const stringImpl& workerName = "DVD_WORKER_");
    void shutdown();

    void flushCallbackQueue();
    void waitForAllTasks(bool yield, bool flushCallbacks, bool forceClear = false);

    Task* createTask(Task* parentTask,
                     I64 jobIdentifier,
                     const DELEGATE_CBK<void, const Task&>& threadedFunction,
                     const DELEGATE_CBK<void>& onCompletionFunction);

    inline U32 workerThreadCount() const noexcept {
        return _workerThreadCount;
    }

  private:
    //ToDo: replace all friend class declarations with attorneys -Ionut;
    friend class Task;
    friend struct TaskHandle;
    void taskStarted(size_t poolIndex);
    void taskCompleted(size_t poolIndex, bool runCallback);
    
    inline ThreadPool& threadPool() {
        assert(_mainTaskPool != nullptr);
        return *_mainTaskPool;
    }

    void nameThreadpoolWorkers(const char* name, ThreadPool& pool);
    void runCbkAndClearTask(size_t taskIndex);

    TaskState state(size_t index) const;

  private:
    std::unique_ptr<ThreadPool> _mainTaskPool;
    boost::lockfree::queue<size_t> _threadedCallbackBuffer;

    vector<Task> _tasksPool;
    vector<TaskState> _taskStates;
    vector<DELEGATE_CBK<void>> _taskCallbacks;

    mutable SharedLock _taskStateLock;

    std::atomic<size_t> _allocatedJobs;

    U32 _workerThreadCount;
};

TaskHandle CreateTask(TaskPool& pool,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction = DELEGATE_CBK<void>());

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction = DELEGATE_CBK<void>());

TaskHandle CreateTask(TaskPool& pool,
                     I64 jobIdentifier,
                     const DELEGATE_CBK<void, const Task&>& threadedFunction,
                     const DELEGATE_CBK<void>& onCompletionFunction = DELEGATE_CBK<void>());

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      I64 jobIdentifier,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction = DELEGATE_CBK<void>());

TaskHandle parallel_for(TaskPool& pool,
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority = Task::TaskPriority::DONT_CARE,
                        U32 taskFlags = 0);

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks, bool foceClear);

};

#endif _TASK_POOL_H_