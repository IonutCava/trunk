#include "stdafx.h"

#include "Headers/Task.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

void finish(Task& task) {
    if (task._unfinishedJobs.fetch_sub(1, std::memory_order_relaxed) <= 1) {
        if (task._parent != nullptr) {
            finish(*task._parent);
        }
#if defined(DEBUG_TASK_SYSTEM)
        task._childTasks.clear();
#endif
    }
}

bool run(Task& task, TaskPool& pool, TaskPriority priority, bool wait, DELEGATE_CBK<void> onCompletionFunction) {

    while (task._unfinishedJobs.load(std::memory_order_relaxed) > 1) {
        if (wait) {
            task._parentPool->threadWaiting();
        } else {
            return false;
        }
    }
    
    if (task._callback) {
        task._callback(task);
    }

    pool.taskCompleted(task._id, priority, onCompletionFunction);
    finish(task);
    return true;
}

Task& Start(Task& task, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction) {
    if (!task._parentPool->enqueue([&task, priority, onCompletionFunction](bool wait) {
                                        return run(task, *task._parentPool, priority, wait, onCompletionFunction);
                                    },
                                    priority))
    {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        run(task, *task._parentPool, priority, true, onCompletionFunction);
    }

    return task;
}

void Wait(const Task& task) {
    while (!Finished(task)) {
        TaskYield(task);
    }
}

Task& Stop(Task& task) {
    task._stopRequested.store(true);
    return task;
}

bool StopRequested(const Task& task) {
    return task._stopRequested.load() ||
           task._parentPool->stopRequested();
}

bool Finished(const Task& task) {
    return task._unfinishedJobs.load(std::memory_order_relaxed) == 0;
}

void TaskYield(const Task& task) {
    task._parentPool->threadWaiting();
}

};