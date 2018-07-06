#include "stdafx.h"

#include "Headers/Task.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

namespace {
    const bool g_DebugTaskStartStop = false;
};

typedef DELEGATE_CBK<void, size_t> OnFinishCbk;

void finish(Task* task) {
    if (task->_unfinishedJobs.fetch_sub(1) == 1) {
        //task->_taskDoneCV.notify_one();
        if (task->_parent != nullptr) {
           finish(task->_parent);
        }
    }
}

void run(Task* task, const OnFinishCbk& onFinish) {
    //UniqueLock lk(_taskDoneMutex);
    //_taskDoneCV.wait(lk, [this]() -> bool { return _unfinishedJobs.load() == 1; });
    while (task->_unfinishedJobs.load() > 1) {
        std::this_thread::yield();
    }

    if (task->_callback) {
        task->_callback(*task);
    }

    onFinish(task->_id);

    finish(task);
}


void runTaskWithDebugInfo(Task* task, const OnFinishCbk& onFinish) {
    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), task->_id.load(), std::this_thread::get_id());
    run(task, onFinish);
    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), task->_id.load(), std::this_thread::get_id());
}

PoolTask getRunTask(Task* task, const OnFinishCbk& onFinish, TaskPriority priority, U32 taskFlags) {
    if (BitCompare(taskFlags, to_base(TaskFlags::PRINT_DEBUG_INFO))) {
        return PoolTask([task, onFinish]() { runTaskWithDebugInfo(task, onFinish); });
    }

    return PoolTask([task, onFinish]() { run(task, onFinish); });
}

void Start(Task* task, TaskPool& pool, TaskPriority priority, U32 taskFlags) {
    assert(task != nullptr);

    if (Config::USE_SINGLE_THREADED_TASK_POOLS) {
        if (priority != TaskPriority::REALTIME) {
            priority = TaskPriority::REALTIME;
        }
    }

    if (g_DebugTaskStartStop) {
        SetBit(taskFlags, to_base(TaskFlags::PRINT_DEBUG_INFO));
    }

    bool runCallback = priority == TaskPriority::REALTIME;
    OnFinishCbk onFinish = [&pool, runCallback](size_t index) {
        pool.taskCompleted(index, runCallback);
    };

    PoolTask wrappedTask(getRunTask(task, onFinish, priority, taskFlags));

    if (priority != TaskPriority::REALTIME) {
        while (!pool.enqueue(wrappedTask)) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        }
    } else {
        wrappedTask();
    }
}

void Wait(const Task* task) {
    /*UniqueLock lk(task->_taskDoneMutex);
    task->_taskDoneCV.wait(lk, [task]() -> bool { return task->_unfinishedJobs.load() == 0; });*/
    while (!Finished(task)) {
        std::this_thread::yield();
    }
}

void Stop(Task* task) {
    task->_stopRequested.store(true);
}

bool StopRequested(const Task *task) {
    return task->_stopRequested.load() ||
           task->_parentPool->stopRequested();
}

bool Finished(const Task *task) {
    return task->_unfinishedJobs.load() == 0;
}

TaskHandle& TaskHandle::startTask(TaskPriority prio, U32 taskFlags) {
    assert(_task->_unfinishedJobs.load() > 0 && "StartTask error: double start call detected!");

    Start(_task, *_tp, prio, taskFlags);
    return *this;
}

};