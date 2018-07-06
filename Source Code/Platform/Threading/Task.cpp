#include "Headers/Task.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    const bool g_DebugTaskStartStop = false;
};

Task::Task()
    : GUIDWrapper(),
    _application(Application::instance()),
    _tp(nullptr),
    _poolIndex(0),
    _jobIdentifier(-1),
    _priority(TaskPriority::DONT_CARE)
{
    _parentTask = nullptr;
    _childTaskCount = 0;
    _done = true;
    _stopRequested = false;
    _childTasks.fill(nullptr);
}

Task::~Task()
{
    stopTask();
    wait();
}

void Task::reset() {
    if (!_done) {
        stopTask();
        wait();
    }

    _stopRequested = false;
    _callback = [](const Task&){};
    _jobIdentifier = -1;
    _priority = TaskPriority::DONT_CARE;
    _parentTask = nullptr;
    _childTasks.fill(nullptr);
    _childTaskCount = 0;
}

void Task::addChildTask(Task* task) {
    DIVIDE_ASSERT(!isRunning(),
                  "Task::addChildTask error: Can't add child tasks to running tasks!");

    assert(_childTaskCount < MAX_CHILD_TASKS);
    task->_parentTask = this;
    _childTasks[_childTaskCount++] = task;
}

void Task::runTaskWithGPUSync() {
    beginSyncGPU();
    run();
    endSyncGPU();
}

void Task::runTaskWithDebugInfo() {
    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());
    run();
    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
}

void Task::runTaskWithGPUSyncAndDebugInfo() {
    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());
    runTaskWithGPUSync();
    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
}

PoolTask Task::getRunTask(TaskPriority priority, U32 taskFlags) {
    if (BitCompare(taskFlags, to_const_U32(TaskFlags::PRINT_DEBUG_INFO))) {
        if (BitCompare(taskFlags, to_const_U32(TaskFlags::SYNC_WITH_GPU))) {
            return PoolTask(to_const_U32(priority), [this]() { runTaskWithGPUSyncAndDebugInfo(); });
        } else {
            return PoolTask(to_const_U32(priority), [this]() { runTaskWithDebugInfo(); });
        }
    } else {
        if (BitCompare(taskFlags, to_const_U32(TaskFlags::SYNC_WITH_GPU))) {
            return PoolTask(to_const_U32(priority), [this]() { runTaskWithGPUSync(); });
        }
    }

    return PoolTask(to_const_U32(priority), [this]() { run(); });
}

void Task::startTask(TaskPriority priority, U32 taskFlags) {
    if (g_DebugTaskStartStop) {
        SetBit(taskFlags, to_const_U32(TaskFlags::PRINT_DEBUG_INFO));
    }

    assert(!isRunning());

    if (Config::USE_SINGLE_THREADED_TASK_POOLS) {
        if (priority != TaskPriority::REALTIME &&
            priority != TaskPriority::REALTIME_WITH_CALLBACK) {
            priority = TaskPriority::REALTIME_WITH_CALLBACK;
        }
    }

    if (!Config::USE_GPU_THREADED_LOADING) {
        if (BitCompare(taskFlags, to_const_U32(TaskFlags::SYNC_WITH_GPU))) {
            priority = TaskPriority::REALTIME_WITH_CALLBACK;
            ClearBit(taskFlags, to_const_U32(TaskFlags::SYNC_WITH_GPU));
        }
    }

    _done = false;
    _priority = priority;

    if (priority != TaskPriority::REALTIME && 
        priority != TaskPriority::REALTIME_WITH_CALLBACK &&
        _tp != nullptr && _tp->workerThreadCount() > 0)
    {
        U32 failCount = 0;
        while (!_tp->threadPool().schedule(getRunTask(priority, taskFlags))) {
            ++failCount;
        }
        if (failCount > 0) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")), failCount);
        }
    } else {
        getRunTask(priority, taskFlags)();
    }
}

void Task::stopTask() {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (isRunning()) {
            Console::errorfn(Locale::get(_ID("TASK_DELETE_ACTIVE")));
        }
    }

    for (I8 i = 0; i < _childTaskCount; ++i) {
        _childTasks[i]->stopTask();
    }

    _stopRequested = true;
}

void Task::wait() {
    std::unique_lock<std::mutex> lk(_taskDoneMutex);
    _taskDoneCV.wait(lk,
                     [this]() -> bool {
                        return _done;
                     });
}

void Task::run() {
    //ToDo: Add better wait for children system. Just manually balance calls for now -Ionut
    WAIT_FOR_CONDITION(_childTaskCount == 0);

    if (_callback) {
        _callback(*this);
    }

    if (_parentTask != nullptr) {
        _parentTask->_childTaskCount -= 1;
    }

    _done = true;
    {
        std::unique_lock<std::mutex> lk(_taskDoneMutex);
        _taskDoneCV.notify_one();
    }

    // task finished. Everything else is bookkeeping
    _tp->taskCompleted(poolIndex(), _priority);
}

void Task::beginSyncGPU() {
    Attorney::ApplicationTask::syncThreadToGPU(_application, std::this_thread::get_id(), true);
}

void Task::endSyncGPU() {
    Attorney::ApplicationTask::syncThreadToGPU(_application, std::this_thread::get_id(), false);
}

};