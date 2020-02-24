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
    const U16 runningCount = task._unfinishedJobs.fetch_sub(1, std::memory_order_relaxed);

    if (runningCount == 1) {
        if (task._parent != nullptr) {
            finish(*task._parent);
        }
    }
}

Task& Start(Task& task, TaskPriority priority, DELEGATE_CBK<void>&& onCompletionFunction) {
    const bool hasOnCompletionFunction = (priority != TaskPriority::REALTIME && onCompletionFunction);

    const auto run = [&task, priority, hasOnCompletionFunction](bool wait) {
        while (task._unfinishedJobs.load(std::memory_order_relaxed) > 1) {
            if (wait) {
                TaskYield(task);
            } else {
                return false;
            }
        }
        if (wait && !task._runWhileIdle) {
            return false;
        }

        if (task._callback) {
            task._callback(task);
        }

        finish(task);
        task._parentPool->taskCompleted(task._id, priority, hasOnCompletionFunction);
        return true;
    };

    if (!task._parentPool->enqueue(run, priority, task._id, std::forward<DELEGATE_CBK<void>>(onCompletionFunction))) {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        run(true);
        if (hasOnCompletionFunction) {
            onCompletionFunction();
        }
    }

    return task;
}

void Wait(const Task& task) {
    if (TaskPool::USE_OPTICK_PROFILER) {
        OPTICK_EVENT();
    }

    while (!Finished(task)) {
        TaskYield(task);
    }
}

Task& Stop(Task& task) noexcept {
    task._stopRequested.store(true);
    return task;
}

bool StopRequested(const Task& task) noexcept {
    return task._stopRequested.load() ||
           task._parentPool->stopRequested();
}

bool Finished(const Task& task) noexcept {
    return task._unfinishedJobs.load(std::memory_order_relaxed) == 0;
}

void TaskYield(const Task& task) {
    task._parentPool->threadWaiting();
}

};