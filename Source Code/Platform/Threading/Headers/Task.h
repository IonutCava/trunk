/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _TASKS_H_
#define _TASKS_H_

#include "Platform/Threading/Headers/Thread.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Application;
class TaskPool;
/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, private NonCopyable {
   public:
       static constexpr U16 MAX_CHILD_TASKS = to_const_ushort(std::numeric_limits<I8>::max());

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
        SYNC_WITH_GPU = toBit(1),
        PRINT_DEBUG_INFO = toBit(2),
        COUNT = 2,
      };

    /**
     * @brief Creates a new Task that runs in a separate thread
     * @param f The callback function (bool param will be true if the task receives a stop request)
     */
    explicit Task();
    ~Task();

    void startTask(TaskPriority priority = TaskPriority::DONT_CARE,
                   U32 taskFlags = 0);

    void stopTask();

    void reset();

    inline void setOwningPool(TaskPool& pool, U32 poolIndex) {
        _tp = &pool;
        _poolIndex = poolIndex;
    }

    inline TaskPool& getOwningPool() {
        assert(_tp != nullptr);
        return *_tp;
    }

    inline bool isRunning() const {
        return !_done;
    }

    inline I64 jobIdentifier() const {
        return _jobIdentifier;
    }

    inline U32 poolIndex() const {
        return _poolIndex;
    }

    inline bool stopRequested() const {
        return _stopRequested;
    }

    inline void threadedCallback(const DELEGATE_CBK_PARAM<const Task&>& cbk, I64 jobIdentifier) {
        _callback = cbk;
        _jobIdentifier = jobIdentifier;
    }

    void addChildTask(Task* task);

    void wait();

   protected:
    void run();
    void runTaskWithGPUSync();
    void runTaskWithDebugInfo();
    void runTaskWithGPUSyncAndDebugInfo();

    PoolTask getRunTask(TaskPriority priority, U32 taskFlags);

    void beginSyncGPU();
    void endSyncGPU();

   private:
    const Application& _application;

    mutable std::mutex _taskDoneMutex;
    std::condition_variable _taskDoneCV;
    std::atomic_bool _done;

    I64 _jobIdentifier;

    std::atomic_bool _stopRequested;

    DELEGATE_CBK_PARAM<const Task&> _callback;
    
    TaskPriority _priority;
    TaskPool* _tp;
    U32 _poolIndex;

    Task* _parentTask;
    std::array<Task*, MAX_CHILD_TASKS> _childTasks;
    std::atomic_short _childTaskCount;
};

// A task object may be used for multiple jobs
struct TaskHandle {
    explicit TaskHandle(Task* task, I64 id) 
        : _task(task),
          _jobIdentifier(id)
    {
    }

    inline void startTask(Task::TaskPriority prio = Task::TaskPriority::DONT_CARE,
                          U32 taskFlags = 0) {
        assert(_task != nullptr);
        _task->startTask(prio, taskFlags);
    }

    inline Task* addChildTask(Task* task) {
        assert(_task != nullptr);
        _task->addChildTask(task);
        return task;
    }

    inline void wait() {
        if (_task != nullptr) {
            _task->wait();
        }
    }

    Task* _task;
    I64 _jobIdentifier;
};


};  // namespace Divide

#endif
