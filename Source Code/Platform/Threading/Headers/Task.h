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

#include "SharedMutex.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, public std::enable_shared_from_this<Task> {
   public:
       enum class TaskPriority : U32 {
           DONT_CARE = 0,
           LOW = 1,
           MEDIUM = 2,
           HIGH = 3,
           MAX = 4,
           REALTIME = 5 //<= not threaded
       };
    /**
     * @brief Creates a new Task that runs in a separate thread
     * @param f The callback function (bool param will be true if the task receives a stop request)
     */
    explicit Task(ThreadPool& tp);
    ~Task();

    Task(const Task& old);

    void startTask(TaskPriority priority = TaskPriority::DONT_CARE);

    void stopTask();

    void reset();

    inline bool isRunning() const {
        return !_done;
    }

    inline I64 jobIdentifier() const {
        return _jobIdentifier;
    }

    inline void onCompletionCbk(const DELEGATE_CBK_PARAM<I64>& cbk) {
        _onCompletionCbk = cbk;
    }

    inline void threadedCallback(const DELEGATE_CBK_PARAM<bool>& cbk, I64 jobIdentifier) {
        _callback = cbk;
        _jobIdentifier = jobIdentifier;
    }

    void addChildTask(Task* task) {
        task->_parentTask = this;
        _childTasks.push_back(task);
        _childTaskCount += 1;
    }

    void wait();

   protected:
    void run();
    void waitForChildren(bool yeld, I64 timeout);

   private:
    mutable std::mutex _taskDoneMutex;
    std::condition_variable _taskDoneCV;
    std::atomic_bool _done;

    std::atomic_bool _stopRequested;
    std::atomic<I64> _jobIdentifier;

    DELEGATE_CBK_PARAM<const std::atomic_bool&> _callback;
    DELEGATE_CBK_PARAM<I64> _onCompletionCbk;
    
    TaskPriority _priority;
    ThreadPool& _tp;

    Task* _parentTask;
    vectorImpl<Task*> _childTasks;
    std::atomic<I64> _childTaskCount;
};

// A task object may be used for multiple jobs
struct TaskHandle {
    explicit TaskHandle(Task* task, I64 id) 
        : _task(task),
          _jobIdentifier(id)
    {
    }

    inline void startTask(Task::TaskPriority prio = Task::TaskPriority::DONT_CARE) {
        _task->startTask(prio);
    }

    inline Task* addChildTask(Task* task) {
        _task->addChildTask(task);
        return task;
    }

    inline void wait() {
        _task->wait();
    }

    Task* _task;
    I64 _jobIdentifier;
};


};  // namespace Divide

#endif
