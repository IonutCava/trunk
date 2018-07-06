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

TaskHandle& TaskHandle::startTask(Task::TaskPriority prio, U32 taskFlags) {
    if (Config::USE_SINGLE_THREADED_TASK_POOLS) {
        if (prio != Task::TaskPriority::REALTIME) {
            prio = Task::TaskPriority::REALTIME;
        }
    }

    assert(_tp != nullptr && _task != nullptr);
    _tp->taskStarted(_task->poolIndex());
    _task->startTask(*_tp, prio, taskFlags);
    return *this;
}

Task::Task()
    : Task(TaskDescriptor())
{
    _unfinishedJobs.fetch_sub(1);
}

Task::Task(const TaskDescriptor& descriptor)
   : GUIDWrapper(),
    _parent(descriptor.parentTask),
    _poolIndex(descriptor.index),
    _callback(descriptor.cbk),
    _unfinishedJobs{ 1 } //1 == this task
{
    _stopRequested.store(false);
    if (_parent != nullptr) {
        _parent->_unfinishedJobs.fetch_add(1);
    }
}

Task::~Task()
{
    stopTask();
    wait();
}

void Task::runTaskWithDebugInfo() {
    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());
    run();
    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
}

PoolTask Task::getRunTask(TaskPriority priority, U32 taskFlags) {
    if (BitCompare(taskFlags, to_base(TaskFlags::PRINT_DEBUG_INFO))) {
         return PoolTask(to_base(priority), [this]() { runTaskWithDebugInfo(); });
    }

    return PoolTask(to_base(priority), [this]() { run(); });
}

void Task::startTask(TaskPool& pool, TaskPriority priority, U32 taskFlags) {
    if (g_DebugTaskStartStop) {
        SetBit(taskFlags, to_base(TaskFlags::PRINT_DEBUG_INFO));
    }

    bool runCallback = priority == TaskPriority::REALTIME;
    _onFinish = [&pool, runCallback](size_t index) {
        pool.taskCompleted(index, runCallback);
    };

    if (priority != TaskPriority::REALTIME)
    {
        PoolTask task(getRunTask(priority, taskFlags));
        while (!pool.threadPool().enqueue(task)) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        }
    } else {
        getRunTask(priority, taskFlags)();
    }
}

void Task::stopTask() {
    _stopRequested.store(true);
}

void Task::wait() {
    UniqueLock lk(_taskDoneMutex);
    _taskDoneCV.wait(lk, [this]() -> bool { return finished(); });
}

void Task::run() {
    _taskThread = std::this_thread::get_id();
    {
        UniqueLock lk(_childTaskMutex);
        _childTaskCV.wait(lk, [this]() -> bool { return _unfinishedJobs.load() == 1; });
    }

    if (_callback) {
        _callback(*this);
    }

    _onFinish(_poolIndex);

    if (_parent != nullptr) {
        _parent->_unfinishedJobs.fetch_sub(1);
    }
    _unfinishedJobs.fetch_sub(1);

    _taskDoneCV.notify_one();
}

bool Task::finished() const {
    return _unfinishedJobs.load() == 0;
}

};