#include "Headers/Task.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    const bool g_DebugTaskStartStop = false;
};

Task::Task(ThreadPool& tp) : GUIDWrapper(),
                             _tp(tp),
                             _jobIdentifier(-1),
                             _priority(TaskPriority::DONT_CARE)
{
    _parentTask = nullptr;
    _childTaskCount = 0;
    _done = true;
    _stopRequested = false;
}

Task::Task(const Task& old) : GUIDWrapper(old),
                              _tp(old._tp)
{
    _stopRequested.store(old._stopRequested);
    _jobIdentifier.store(old._jobIdentifier);
    _callback = old._callback;
    _onCompletionCbk = old._onCompletionCbk;
    _priority = old._priority;
    _childTaskCount.store(old._childTaskCount);
    _parentTask = old._parentTask;
    _childTasks.insert(std::cend(_childTasks), std::cbegin(old._childTasks), std::cend(old._childTasks));
    _done.store(old._done);
}

Task::~Task()
{
    stopTask();
    wait();
}

void Task::reset() {
    stopTask();
    wait();

    _stopRequested = false;
    _callback = DELEGATE_CBK_PARAM<bool>();
    _onCompletionCbk = DELEGATE_CBK_PARAM<I64>();
    _jobIdentifier = -1;
    _priority = TaskPriority::DONT_CARE;
    _parentTask = nullptr;
    _childTasks.clear();
    _childTaskCount = 0;
}

void Task::startTask(TaskPriority priority) {
    assert(!isRunning());

    _done = false;
    _priority = priority;
    if (priority != TaskPriority::REALTIME) {
        while (!_tp.schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this)))) {
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
    if (g_DebugTaskStartStop) {
        Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());
    }

    if (!Application::getInstance().ShutdownRequested()) {

        if (_callback) {
            _callback(_stopRequested);
        }

        if (_onCompletionCbk) {
            _onCompletionCbk(getGUID());
        }
    }

    if (_parentTask != nullptr) {
        _parentTask->_childTaskCount -= 1;
    }

    if (g_DebugTaskStartStop) {
        Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
    }

    _done = true;

    std::unique_lock<std::mutex> lk(_taskDoneMutex);
    _taskDoneCV.notify_one();
}

};