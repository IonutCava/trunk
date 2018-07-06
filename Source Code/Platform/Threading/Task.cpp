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


Task::Task()
    : GUIDWrapper(),
    _tp(nullptr),
    _poolIndex(0),
    _jobIdentifier(-1),
    _priority(TaskPriority::DONT_CARE)
{
    _parentTask = nullptr;
    _stopRequested = false;
    _childTasks.reserve(MAX_CHILD_TASKS);
}

Task::~Task()
{
    stopTask();
    wait();
}

bool Task::isRunning() const {
    if (_tp) {
        return _tp->state(poolIndex()) != TaskPool::TaskState::TASK_FREE;
    }

    return false;
}

void Task::reset() {
    if (_tp->state(poolIndex()) == TaskPool::TaskState::TASK_RUNNING) {
        stopTask();
        wait();
    }

    _stopRequested = false;
    _callback = nullptr;
    _jobIdentifier = -1;
    _priority = TaskPriority::DONT_CARE;
    _parentTask = nullptr;
    _taskThread = std::thread::id();
    _childTasks.resize(0);
}

Task* Task::addChildTask(Task* task) {
    DIVIDE_ASSERT(_tp->state(poolIndex()) == TaskPool::TaskState::TASK_ALLOCATED,
                  "Task::addChildTask error: Can't add child tasks to running or free tasks!");

    assert(childTaskCount() < MAX_CHILD_TASKS && task->_parentTask == nullptr);
    task->_parentTask = this; 

    UniqueLock u_lock(_childTaskMutex);
    _childTasks.push_back(task);

    return _childTasks.back();
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

void Task::startTask(TaskPriority priority, U32 taskFlags) {
    if (_tp->state(poolIndex()) == TaskPool::TaskState::TASK_RUNNING) {
        return;
    }

    if (g_DebugTaskStartStop) {
        SetBit(taskFlags, to_base(TaskFlags::PRINT_DEBUG_INFO));
    }

    if (Config::USE_SINGLE_THREADED_TASK_POOLS) {
        if (priority != TaskPriority::REALTIME &&
            priority != TaskPriority::REALTIME_WITH_CALLBACK) {
            priority = TaskPriority::REALTIME_WITH_CALLBACK;
        }
    }

    _priority = priority;

    if (priority != TaskPriority::REALTIME && 
        priority != TaskPriority::REALTIME_WITH_CALLBACK &&
        _tp != nullptr && _tp->workerThreadCount() > 0)
    {
        while (!_tp->threadPool().enqueue(getRunTask(priority, taskFlags))) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), 1);
        }
    } else {
        getRunTask(priority, taskFlags)();
    }
}

void Task::stopTask() {
    {
        UniqueLock u_lock(_childTaskMutex);
        for (Task* child : _childTasks) {
            child->stopTask();
        }
    }

    if (Config::Build::IS_DEBUG_BUILD) {
        if (_tp && _tp->state(poolIndex()) == TaskPool::TaskState::TASK_RUNNING) {
            Console::errorfn(Locale::get(_ID("TASK_DELETE_ACTIVE")));
        }
    }

    _stopRequested = true;
}

vectorAlg::vecSize Task::childTaskCount() const {
    UniqueLock u_lock(_childTaskMutex);
    return _childTasks.size();
}

void Task::removeChildTask(const Task& child) {
    I64 targetGUID = child.getGUID();

    {
        UniqueLock lk(_childTaskMutex);
        _childTasks.erase(
            std::remove_if(std::begin(_childTasks),
                           std::end(_childTasks),
                           [&targetGUID](Task* const childTask) -> bool {
                               return childTask->getGUID() == targetGUID;
                           }),
            std::end(_childTasks));
    }
    _childTaskCV.notify_one();
}

void Task::wait() {
    UniqueLock lk(_taskDoneMutex);
    _taskDoneCV.wait(lk,
                     [this]() -> bool {
                        if (_tp) {
                            return _tp->state(poolIndex()) == TaskPool::TaskState::TASK_FREE;
                        }
                        return true;
                     });
}

void Task::run() {
    _taskThread = std::this_thread::get_id();

    {
        UniqueLock lk(_childTaskMutex);
        _childTaskCV.wait(lk,
                         [this]() -> bool {
                            return _childTasks.empty();
                        });
    }

    {
        UniqueLock lk(_taskDoneMutex);
        _tp->taskStarted(poolIndex(), _priority);
    }

    if (_callback) {
        _callback(*this);
    }

    if (_parentTask != nullptr) {
        _parentTask->removeChildTask(*this);
    }

    // task finished. Everything else is bookkeeping
    {
        UniqueLock lk(_taskDoneMutex);
        _tp->taskCompleted(poolIndex(), _priority);
    }
    _taskDoneCV.notify_one();
}

};