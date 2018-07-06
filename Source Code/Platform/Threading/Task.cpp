#include "Headers/Task.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
Task::Task(ThreadPool& tp) : GUIDWrapper(),
                             _tp(tp),
                             _jobIdentifier(-1),
                             _priority(TaskPriority::DONT_CARE)
{
    _parentTask = nullptr;
    _childTaskCount = 0;
    _done = true;
    _isRunning = false;
    _stopRequested = false;
}

Task::Task(const Task& old) : GUIDWrapper(old),
                              _tp(old._tp)
{
    _done.store(old._done);
    _stopRequested.store(old._stopRequested);
    _jobIdentifier.store(old._jobIdentifier);
    _callback = old._callback;
    _onCompletionCbk = old._onCompletionCbk;
    _priority = old._priority;
    _childTaskCount.store(old._childTaskCount);
    _isRunning.store(old._isRunning);
    _parentTask = old._parentTask;
    _childTasks.insert(std::cend(_childTasks), std::cbegin(old._childTasks), std::cend(old._childTasks));
}

Task::~Task()
{
    if (_done == false) {
        stopTask();
        Console::errorfn(Locale::get(_ID("TASK_DELETE_ACTIVE")));
    }

    WAIT_FOR_CONDITION(_done || _tp.active() == 0);
}

bool Task::reset() {
    bool wasNeeded = false;
    if (!_done) {
        wasNeeded = true;
        stopTask();
        WAIT_FOR_CONDITION(_done);
    }
    _isRunning = false;
    _stopRequested = false;
    _callback = DELEGATE_CBK_PARAM<bool>();
    _onCompletionCbk = DELEGATE_CBK_PARAM<I64>();
    _jobIdentifier = -1;
    _priority = TaskPriority::DONT_CARE;
    _parentTask = nullptr;
    _childTasks.clear();
    _childTaskCount = 0;

    return wasNeeded;
}

void Task::startTask(TaskPriority priority) {
    assert(!isRunning());

    _done = false;
    _isRunning = true;
    _priority = priority;

    while (!_tp.schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this)))) {
        Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")));
    }
}

void Task::stopTask() {
    for (Task* child : _childTasks){
        child->stopTask();
    }

    _stopRequested = true;
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
            if (endTime - startTime >= timeout) {
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

    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());

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

    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());

    _done = true;
    _isRunning = false;
}

};