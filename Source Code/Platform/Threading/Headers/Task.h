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

struct Task;
class TaskPool;

enum class TaskPriority : U8 {
    DONT_CARE = 0,
    REALTIME = 1, //<= not threaded
    COUNT
};

enum class TaskFlags : U8 {
    PRINT_DEBUG_INFO = toBit(1),
    COUNT = 1,
};

struct alignas(64) Task {
    //mutable std::mutex _taskDoneMutex;
    //mutable std::condition_variable _taskDoneCV;

    U32 _id;
    Task* _parent = nullptr;
    TaskPool* _parentPool = nullptr;
    std::atomic_ushort _unfinishedJobs;
    std::atomic_bool _stopRequested = false;
    DELEGATE_CBK<void, const Task&> _callback;
};

void Start(Task *task, TaskPool& pool, TaskPriority priority, U32 taskFlags);
void Stop(Task *task);
void Wait(const Task *task);
bool StopRequested(const Task *task);
bool Finished(const Task *task);

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

    TaskHandle& startTask(TaskPriority prio = TaskPriority::DONT_CARE, U32 taskFlags = 0);

    inline TaskHandle& wait() {
        if (_task != nullptr) {
            Wait(_task);
        }
        return *this;
    }

    inline I64 jobIdentifier() const {
        return _jobIdentifier;
    }

    inline TaskPool& getOwningPool() {
        return *_tp;
    }

    inline bool taskRunning() const {
        return !Finished(_task);
    }

    Task* _task;
    TaskPool* _tp;
    I64 _jobIdentifier;
};


};  // namespace Divide

#endif
