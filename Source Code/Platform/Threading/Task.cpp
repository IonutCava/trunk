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
    _taskFlags(0),
    _tp(nullptr),
    _poolIndex(0),
    _jobIdentifier(-1),
    _priority(TaskPriority::DONT_CARE)
{
    _parentTask = nullptr;
    _childTaskCount = 0;
    _done = true;
    _stopRequested = false;

    if (g_DebugTaskStartStop) {
        SetBit(_taskFlags, to_const_uint(TaskFlags::PRINT_DEBUG_INFO));
    }
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
    //_callback = DELEGATE_CBK_PARAM<bool>();
    _jobIdentifier = -1;
    _priority = TaskPriority::DONT_CARE;
    _parentTask = nullptr;
    _childTasks.resize(0);
    _childTaskCount = 0;
}

void Task::startTask(TaskPriority priority, U32 taskFlags) {
    SetBit(_taskFlags, taskFlags);

    assert(!isRunning());

    if (Config::USE_SINGLE_THREADED_TASK_POOLS) {
        if (priority != TaskPriority::REALTIME &&
            priority != TaskPriority::REALTIME_WITH_CALLBACK) {
            priority = TaskPriority::REALTIME_WITH_CALLBACK;
        }
    }

    if (!Config::USE_GPU_THREADED_LOADING) {
        if (BitCompare(_taskFlags, to_const_uint(TaskFlags::SYNC_WITH_GPU))) {
            priority = TaskPriority::REALTIME_WITH_CALLBACK;
            ClearBit(_taskFlags, to_const_uint(TaskFlags::SYNC_WITH_GPU));
        }
    }

    _done = false;
    _priority = priority;
    if (priority != TaskPriority::REALTIME && 
        priority != TaskPriority::REALTIME_WITH_CALLBACK &&
        _tp != nullptr && _tp->workerThreadCount() > 0)
    {
        while (!_tp->threadPool().schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this)))) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")));
        }
    } else {
        run();
    }
}

void Task::stopTask() {
#if defined(_DEBUG)
    if (isRunning()) {
        Console::errorfn(Locale::get(_ID("TASK_DELETE_ACTIVE")));
    }
#endif

    for (Task* child : _childTasks){
        child->stopTask();
    }

    _stopRequested = true;
}

void Task::wait() {
    std::unique_lock<std::mutex> lk(_taskDoneMutex);
    while (!_done) {
        _taskDoneCV.wait(lk);
    }
}

//ToDo: Add better wait for children system. Just manually balance calls for now -Ionut
void Task::waitForChildren(bool yeld, I64 timeout) {
    U64 startTime = 0UL;
    if (timeout > 0L) {
        startTime = Time::ElapsedMicroseconds(true);
    }

    while(_childTaskCount > 0) {
        if (timeout > 0L) {
            U64 endTime = Time::ElapsedMicroseconds(true);
            if (endTime - startTime >= static_cast<U64>(timeout)) {
                return;
            }
        } else if (timeout == 0L) {
            return;
        }

        if (yeld) {
            std::this_thread::yield();
        }
    }
}

void Task::run() {
    waitForChildren(true, -1L);

    if (BitCompare(_taskFlags, to_const_uint(TaskFlags::PRINT_DEBUG_INFO))) {
        Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());
    }

    if (BitCompare(_taskFlags, to_const_uint(TaskFlags::SYNC_WITH_GPU))) {
        beginSyncGPU();
    }

    if (!Application::instance().ShutdownRequested()) {

        if (_callback) {
            _callback(_stopRequested);
        }
    }

    if (_parentTask != nullptr) {
        _parentTask->_childTaskCount -= 1;
    }

    _done = true;

    std::unique_lock<std::mutex> lk(_taskDoneMutex);
    _taskDoneCV.notify_one();

    if (BitCompare(_taskFlags, to_const_uint(TaskFlags::SYNC_WITH_GPU))) {
        endSyncGPU();
    }

    if (BitCompare(_taskFlags, to_const_uint(TaskFlags::PRINT_DEBUG_INFO))) {
        Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
    }

    // task finished. Everything else is bookkeeping
    if (_tp != nullptr) {
        _tp->taskCompleted(poolIndex(), _priority);
    }
}

void Task::beginSyncGPU() {
    Attorney::ApplicationTask::syncThreadToGPU(std::this_thread::get_id(), true);
}

void Task::endSyncGPU() {
    Attorney::ApplicationTask::syncThreadToGPU(std::this_thread::get_id(), false);
}

};