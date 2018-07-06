#include "Headers/Task.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Utility/Headers/Localization.h"

#include <chrono>
#include <thread>

namespace Divide {
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
    _done.store(old._done);
    _stopRequested.store(old._stopRequested);
    _jobIdentifier.store(old._jobIdentifier);
    _callback = old._callback;
    _onCompletionCbk = old._onCompletionCbk;
    _priority = old._priority;
    _childTaskCount.store(old._childTaskCount);
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
    _done = false;
    _priority = priority;

    if (_childTaskCount != 0) {
        for (Task* child : _childTasks){
            child->startTask(priority);
        }
    } else {
        if (!_tp.schedule(PoolTask(to_uint(priority), DELEGATE_BIND(&Task::run, this)))) {
            Console::errorfn(Locale::get(_ID("TASK_SCHEDULE_FAIL")));
        }
    }
}

void Task::stopTask() {
    for (Task* child : _childTasks){
        child->stopTask();
    }

    _stopRequested = true;
}

void Task::run() {
    Console::d_printfn(Locale::get(_ID("TASK_RUN_IN_THREAD")), getGUID(), std::this_thread::get_id());

    if (!Application::getInstance().ShutdownRequested()) {

        if (_callback) {
            _callback(_stopRequested);
        }

        if (_onCompletionCbk) {
            _onCompletionCbk(getGUID());
        }

    }

    if (_parentTask != nullptr && (_parentTask->_childTaskCount -= 1) == 0) {
        _parentTask->startTask(_parentTask->_priority);
    }

    Console::d_printfn(Locale::get(_ID("TASK_COMPLETE_IN_THREAD")), getGUID(), std::this_thread::get_id());
    _done = true;
}

};