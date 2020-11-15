#include "stdafx.h"

#include "Headers/Task.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

void Finish(Task& task) {
    if (task._unfinishedJobs.fetch_sub(1, std::memory_order_relaxed) == 1) {
        task._callback = {};
        if (task._parent != nullptr) {
            Finish(*task._parent);
        }
    }
}

void RunLocally(Task& task, TaskPriority priority, const bool hasOnCompletionFunction) {
    while (task._unfinishedJobs.load(std::memory_order_relaxed) > 1) {
        TaskYield(task);
    }
    if (task._callback) {
        task._callback(task);
    }

    Finish(task);
    task._parentPool->taskCompleted(task._id, hasOnCompletionFunction);
};

Task& Start(Task& task, const TaskPriority priority, const DELEGATE<void>& onCompletionFunction) {
    const bool hasOnCompletionFunction = priority != TaskPriority::REALTIME && onCompletionFunction;

    if (!task._parentPool->enqueue(
        [&task, hasOnCompletionFunction](const bool threadWaitingCall)
        {
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

                Finish(task);
                task._parentPool->taskCompleted(task._id, hasOnCompletionFunction);
                return true;
            }

            return false;
        }, 
        priority, 
        task._id,
        onCompletionFunction)) 
    {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        RunLocally(task, priority, hasOnCompletionFunction);
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