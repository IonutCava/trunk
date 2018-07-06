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
#ifndef _TASKS_H_
#define _TASKS_H_

#include "Platform/Threading/Headers/ThreadPool.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Application;

class TaskPool;

/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, protected NonCopyable {
   public:
       static constexpr U16 MAX_CHILD_TASKS = to_base(std::numeric_limits<I8>::max());

       enum class TaskPriority : U32 {
           DONT_CARE = 0,
           LOW = 1,
           MEDIUM = 2,
           HIGH = 3,
           MAX = 4,
           REALTIME = 5, //<= not threaded
           REALTIME_WITH_CALLBACK = 6, //<= not threaded
           COUNT
       };

      enum class TaskFlags : U32 {
        PRINT_DEBUG_INFO = toBit(1),
        COUNT = 1,
      };

    Task();
    ~Task();

    void startTask(TaskPriority priority = TaskPriority::DONT_CARE,
                   U32 taskFlags = 0);

    void stopTask();

    void reset();

    bool isRunning() const;

    inline TaskPool& getOwningPool() noexcept {
        assert(_tp != nullptr);
        return *_tp;
    }

    inline I64 jobIdentifier() const noexcept {
        return _jobIdentifier;
    }

    inline U32 poolIndex() const noexcept {
        return _poolIndex;
    }

    inline bool stopRequested() const noexcept {
        return _stopRequested;
    }

    inline void threadedCallback(const DELEGATE_CBK<void, const Task&>& cbk, I64 jobIdentifier) {
        _callback = cbk;
        _jobIdentifier = jobIdentifier;
    }

    Task* addChildTask(Task* task);

    void wait();

   protected:
    friend class TaskPool;
    inline void setPoolIndex(TaskPool* const pool, U32 index) noexcept {
        _tp = pool;
        _poolIndex = index;
    }
   protected:
    void run();
    void runTaskWithDebugInfo();
    vectorAlg::vecSize childTaskCount() const;
    void removeChildTask(const Task& child);
    PoolTask getRunTask(TaskPriority priority, U32 taskFlags);

   private:
    mutable std::mutex _taskDoneMutex;
    std::condition_variable _taskDoneCV;

    I64 _jobIdentifier;

    std::atomic_bool _stopRequested;

    DELEGATE_CBK<void, const Task&> _callback;
    
    TaskPriority _priority;
    TaskPool* _tp;
    U32 _poolIndex;

    Task* _parentTask;

    mutable std::mutex _childTaskMutex;
    std::condition_variable _childTaskCV;

    vectorImpl<Task*> _childTasks;
    std::atomic<std::thread::id> _taskThread;
};

// A task object may be used for multiple jobs
struct TaskHandle {
    explicit TaskHandle() noexcept
        : TaskHandle(nullptr, -1)
    {
    }

    explicit TaskHandle(Task* task, I64 id)  noexcept
        : _task(task),
          _jobIdentifier(id)
    {
    }

    inline TaskHandle& startTask(Task::TaskPriority prio = Task::TaskPriority::DONT_CARE,
                                 U32 taskFlags = 0) {
        assert(_task != nullptr);
        _task->startTask(prio, taskFlags);
        return *this;
    }

    inline Task* addChildTask(const TaskHandle& taskHandle) {
        Task* task = taskHandle._task;
        assert(_task != nullptr);
        return _task->addChildTask(task);
    }

    inline TaskHandle& wait() {
        if (_task != nullptr) {
            _task->wait();
        }
        return *this;
    }

    Task* _task;
    I64 _jobIdentifier;
};


};  // namespace Divide

#endif
