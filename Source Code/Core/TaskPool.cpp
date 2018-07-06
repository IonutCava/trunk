#include "Headers/TaskPool.h"

namespace Divide {

TaskPool::TaskPool() 
    : _threadedCallbackBuffer(Config::MAX_POOLED_TASKS)
{
    assert(isPowerOfTwo(Config::MAX_POOLED_TASKS));
    _allocatedJobs = 0;

    for (Task& task : _tasksPool) {
        task.setOwningPool(_mainTaskPool);
        hashAlg::emplace(_taskStates, task.getGUID(), false);
    }
}

TaskPool::~TaskPool()
{
    _mainTaskPool.clear();
    WAIT_FOR_CONDITION(_mainTaskPool.active() == 0);

    _mainTaskPool.wait(0);
    _taskStates.clear();
}

bool TaskPool::init() {
    // We have an A.I. thread, a networking thread, a PhysX thread, the main
    // update/rendering thread so how many threads do we allocate for tasks?
    // That's up to the programmer to decide for each app.
    U32 threadCount = HARDWARE_THREAD_COUNT();
    if (threadCount <= 2) {
        return false;;
    }

    _mainTaskPool.size_controller().resize(std::max(threadCount + 1, 2U));

    return true;
}

void TaskPool::idle()
{
    I64 taskGUID = -1;
    while (_threadedCallbackBuffer.pop(taskGUID)) {
        CallbackFunctions::const_iterator it = _taskCallbacks.find(taskGUID);
        if (it != std::cend(_taskCallbacks) && it->second) {
            it->second();
            _taskStates[taskGUID] = false;
        }
    }
}

void TaskPool::setTaskCallback(const TaskHandle& handle,
                               const DELEGATE_CBK<>& callback) {
    if (callback) {
        hashAlg::emplace(_taskCallbacks, handle._task->getGUID(), callback);
    }
}

void TaskPool::taskCompleted(I64 onExitTaskID) {
    CallbackFunctions::const_iterator it = _taskCallbacks.find(onExitTaskID);
    if (it == std::cend(_taskCallbacks)) {
        _taskStates[onExitTaskID] = false;
    }

    WAIT_FOR_CONDITION(_threadedCallbackBuffer.push(onExitTaskID));
    // Signal main thread to execute callback
}

TaskHandle TaskPool::getTaskHandle(I64 taskGUID) {
    for (Task& freeTask : _tasksPool) {
        if (freeTask.getGUID() == taskGUID) {
            return TaskHandle(&freeTask, freeTask.jobIdentifier());
        }
    }
    // return the first task instead of a dummy result
    return TaskHandle(&_tasksPool.front(), -1);
}

Task& TaskPool::getAvailableTask() {
    Task* task = &_tasksPool[(++_allocatedJobs - 1u) & (Config::MAX_POOLED_TASKS - 1u)];
    U32 failCount = 0;
    bool* state = &_taskStates[task->getGUID()];
    while (*state) {
        failCount++;
        assert(failCount < Config::MAX_POOLED_TASKS * 2);
        task = &_tasksPool[(++_allocatedJobs - 1u) & (Config::MAX_POOLED_TASKS - 1u)];
        state = &_taskStates[task->getGUID()];
    }

    *state = true;
    task->reset();
    task->onCompletionCbk(DELEGATE_BIND(&TaskPool::taskCompleted, this, std::placeholders::_1));

    return *task;
}

};
