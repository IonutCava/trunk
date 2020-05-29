#include "stdafx.h"

#include "Headers/Task.h"

#include "Core/Headers/Application.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

void finish(Task& task) {
    if (task._unfinishedJobs.fetch_sub(1, std::memory_order_relaxed) == 1) {
        task._callback = {};
        if (task._parent != nullptr) {
            finish(*task._parent);
        }
    }
}

void runLocally(Task& task, TaskPriority priority, bool hasOnCompletionFunction) {
    while (task._unfinishedJobs.load(std::memory_order_relaxed) > 1) {
        TaskYield(task);
    }
    if (task._callback) {
        task._callback(task);
    }

    finish(task);
    task._parentPool->taskCompleted(task._id, hasOnCompletionFunction);
};

Task& Start(Task& task, TaskPriority priority, DELEGATE<void>&& onCompletionFunction) {
    const bool hasOnCompletionFunction = (priority != TaskPriority::REALTIME && onCompletionFunction);

    const auto run = [&task, hasOnCompletionFunction](bool threadWaitingCall) {
        while (task._unfinishedJobs.load(std::memory_order_relaxed) > 1) {
            if (threadWaitingCall) {
                TaskYield(task);
            } else {
                return false;
            }
        }

        if (!threadWaitingCall || task._runWhileIdle) {
            if (task._callback) {
                task._callback(task);
            }

            finish(task);
            task._parentPool->taskCompleted(task._id, hasOnCompletionFunction);
            return true;
        }

        return false;
    };

    if (!task._parentPool->enqueue(std::move(run), priority, task._id, std::move(onCompletionFunction))) {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        runLocally(task, priority, hasOnCompletionFunction);
        task._parentPool->flushCallbackQueue();
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

bool Finished(const Task& task) noexcept {
    return task._unfinishedJobs.load(std::memory_order_relaxed) == 0;
}

void TaskYield(const Task& task) {
    task._parentPool->threadWaiting();
}

};