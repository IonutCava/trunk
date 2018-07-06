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

class Task;
class TaskPool;
struct TaskDescriptor {
    size_t index = 0;
    Task* parentTask = nullptr;
    DELEGATE_CBK<void, const Task&> cbk;
};
/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, protected NonCopyable {
   public:
       static constexpr U16 MAX_CHILD_TASKS = to_base(std::numeric_limits<I8>::max());

       enum class TaskPriority : U8 {
           DONT_CARE = 0,
           REALTIME = 1, //<= not threaded
           COUNT
       };

      enum class TaskFlags : U8 {
        PRINT_DEBUG_INFO = toBit(1),
        COUNT = 1,
      };

    Task();
    Task(const TaskDescriptor& descriptor);
    ~Task();

    void startTask(TaskPool& pool, TaskPriority priority, U32 taskFlags);

    void stopTask();

    bool finished() const;

    inline size_t poolIndex() const noexcept {
        return _poolIndex;
    }

    inline bool stopRequested() const noexcept {
        if (_parent != nullptr) {
            return _stopRequested.load() || _parent->stopRequested();
        }
        return _stopRequested.load();
    }

    void wait();

   protected:
    void run();
    void runTaskWithDebugInfo();
    PoolTask getRunTask(TaskPriority priority, U32 taskFlags);

   private:
    size_t _poolIndex;
    Task* _parent;
    std::atomic_size_t _unfinishedJobs;

    std::atomic_bool _stopRequested;

    DELEGATE_CBK<void, const Task&> _callback;
    DELEGATE_CBK<void, size_t> _onFinish;

    std::atomic<std::thread::id> _taskThread;

    std::mutex _taskDoneMutex;
    std::condition_variable _taskDoneCV;

    std::mutex _childTaskMutex;
    std::condition_variable _childTaskCV;
};

// A task object may be used for multiple jobs
struct TaskHandle {
    explicit TaskHandle() noexcept
        : TaskHandle(nullptr, nullptr, -1)
    {
    }

    explicit TaskHandle(Task* task, TaskPool* tp, I64 id)  noexcept
        : _task(task),
          _tp(tp),
          _jobIdentifier(id)
    {
    }

    TaskHandle& startTask(Task::TaskPriority prio = Task::TaskPriority::DONT_CARE, U32 taskFlags = 0);

    inline TaskHandle& wait() {
        if (_task != nullptr) {
            _task->wait();
        }
        return *this;
    }

    inline I64 jobIdentifier() const {
        return _jobIdentifier;
    }

    inline TaskPool& getOwningPool() {
        return *_tp;
    }

    Task* _task;
    TaskPool* _tp;
    I64 _jobIdentifier;
};


};  // namespace Divide

#endif
