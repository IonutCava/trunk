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

class TaskPool : public GUIDWrapper {
  public:

    explicit TaskPool();
    ~TaskPool();
    
    bool init(U8 threadCount, bool lockFree, const stringImpl& workerName = "DVD_WORKER");
    void shutdown();

    void flushCallbackQueue();
    void waitForAllTasks(bool yield, bool flushCallbacks, bool forceClear = false);

    Task* createTask(Task* parentTask, const DELEGATE_CBK<void, const Task&>& threadedFunction);

    inline U8 workerThreadCount() const noexcept {
        return _workerThreadCount;
    }

    inline bool operator==(const TaskPool& other) const {
        return getGUID() == other.getGUID();
    }

    inline bool operator!=(const TaskPool& other) const {
        return getGUID() != other.getGUID();
    }

  private:
    //ToDo: replace all friend class declarations with attorneys -Ionut;
    friend struct Task;
    friend struct TaskHandle;
    friend void Start(Task* task, TaskPool& pool, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction);
    friend bool StopRequested(const Task *task);

    void taskCompleted(U32 taskIndex);
    void taskCompleted(U32 taskIndex, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction);
    
    bool enqueue(const PoolTask& task, TaskPriority priority);
    bool stopRequested() const;

    void nameThreadpoolWorkers(const char* name);
    void runCbkAndClearTask(U32 taskIdentifier);

  private:
#if defined(USE_BOOST_ASIO_THREADPOOL)
      std::unique_ptr<boost::asio::thread_pool> _mainTaskPool;
#else
      std::unique_ptr<ThreadPool> _mainTaskPool;
#endif
    boost::lockfree::queue<U32> _threadedCallbackBuffer;
    std::atomic_uint _runningTaskCount;
    std::atomic_bool _stopRequested = false;
    hashMap<U32, DELEGATE_CBK<void>> _taskCallbacks;
    U8 _workerThreadCount;
};

TaskHandle CreateTask(TaskPool& pool,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction);

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction);

TaskHandle parallel_for(TaskPool& pool,
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        TaskPriority priority = TaskPriority::DONT_CARE);

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks, bool foceClear);

};

#endif _TASK_POOL_H_