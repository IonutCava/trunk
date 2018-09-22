#include "stdafx.h"

#include "Headers/Task.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

typedef DELEGATE_CBK<void, U32> OnFinishCbk;

void finish(Task& task) {
    if (task._unfinishedJobs.fetch_sub(1) == 1) {
        if (task._parent != nullptr) {
           finish(*task._parent);
        }
    }
}

void run(Task& task, const OnFinishCbk& onFinish) {

    while (task._unfinishedJobs.load() > 1) {
        std::this_thread::yield();
    }

    if (task._callback) {
        task._callback(task);
        task._callback = 0;
    }

    onFinish(task._id);

    finish(task);
}

void Start(Task& task, TaskPool& pool, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction) {
    PoolTask wrappedTask = 0;
    if (!onCompletionFunction) {
        wrappedTask = [&task, &pool]() {
                          run(task,
                              [&pool](U32 index) {
                                  pool.taskCompleted(index);
                              });
                      };
    } else {
        wrappedTask = [&task, &pool, priority, onCompletionFunction]() {
                        run(task,
                            [&pool, priority, &onCompletionFunction](U32 index) {
                                pool.taskCompleted(index, priority, onCompletionFunction);
                            });
                    };
    }

    while (!pool.enqueue(wrappedTask, priority)) {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
    }
}

void Wait(const Task& task) {
    while (!Finished(task)) {
        std::this_thread::yield();
    }
}

void Stop(Task& task) {
    task._stopRequested.store(true);
}

bool StopRequested(const Task& task) {
    return task._stopRequested.load() ||
           task._parentPool->stopRequested();
}

bool Finished(const Task& task) {
    return task._unfinishedJobs.load() == 0;
}

TaskHandle& TaskHandle::startTask(TaskPriority prio, const DELEGATE_CBK<void>& onCompletionFunction) {
    assert(_task != nullptr);
    assert(_task->_unfinishedJobs.load() > 0 && "StartTask error: double start call detected!");

    Start(*_task, *_tp, prio, onCompletionFunction);
    return *this;
}

bool TaskHandle::operator==(const TaskHandle& other) const {
    if (_tp != nullptr) {
        if (other._tp == nullptr) {
            return false;
        }
    } else if (other._tp != nullptr) {
        return false;
    }
    if (_task != nullptr) {
        if (other._task == nullptr) {
            return false;
        }
    } else if (other._task != nullptr) {
        return false;
    }

    return *_tp == *other._tp && _task->_id == other._task->_id;
}
};