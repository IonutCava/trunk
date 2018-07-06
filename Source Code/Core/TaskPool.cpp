#include "Headers/TaskPool.h"
#include "Headers/Kernel.h"

namespace Divide {

TaskPool::TaskPool() 
    : _threadedCallbackBuffer(Config::MAX_POOLED_TASKS)
{
    assert(isPowerOfTwo(Config::MAX_POOLED_TASKS));
    _allocatedJobs = 0;

    for (Task& task : _tasksPool) {
        task.setOwningPool(*this);
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

void TaskPool::flushCallbackQueue()
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

    return *task;
}

TaskHandle GetTaskHandle(I64 taskGUID) {
    return GetTaskHandle(Application::getInstance()
                                     .kernel()
                                     .taskPool(),
                         taskGUID);
}

TaskHandle GetTaskHandle(TaskPool& pool, I64 taskGUID) {
    return pool.getTaskHandle(taskGUID);
}

TaskHandle CreateTask(const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction)
{
    return CreateTask(-1, threadedFunction, onCompletionFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction)
{
    return CreateTask(pool, -1, threadedFunction, onCompletionFunction);
}

/**
* @brief Creates a new Task that runs in a separate thread
* @param jobIdentifier A unique identifier that gets reset when the job finishes.
*                      Used to check if the local task handle is still valid
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(I64 jobIdentifier,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction)
{
    TaskPool& pool = Application::getInstance().kernel().taskPool();
    return CreateTask(pool, jobIdentifier, threadedFunction, onCompletionFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                   I64 jobIdentifier,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction)
{
    Task& freeTask = pool.getAvailableTask();
    TaskHandle handle(&freeTask, jobIdentifier);

    freeTask.threadedCallback(threadedFunction, jobIdentifier);
    if (onCompletionFunction) {
        pool.setTaskCallback(handle, onCompletionFunction);
    }

    return handle;
}

};
