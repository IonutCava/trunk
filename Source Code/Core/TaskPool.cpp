#include "Headers/TaskPool.h"
#include "Headers/Kernel.h"

namespace Divide {

TaskPool::TaskPool(U32 maxTaskCount)
    : _threadedCallbackBuffer(maxTaskCount),
      _workerThreadCount(0u),
      _tasksPool(maxTaskCount),
      _taskCallbacks(maxTaskCount),
      _taskStates(maxTaskCount, false)
{
    assert(isPowerOfTwo(maxTaskCount));
    _allocatedJobs = 0;

    for (U32 i = 0; i < maxTaskCount; ++i) {
        Task& task = _tasksPool[i];
        task.setOwningPool(*this, i);
    }
}

TaskPool::~TaskPool()
{
    _mainTaskPool.clear();
    WAIT_FOR_CONDITION(_mainTaskPool.active() == 0);

    _mainTaskPool.wait(0);
}

bool TaskPool::init(U32 threadCount) {
    if (threadCount <= 2) {
        return false;
    }

    _workerThreadCount = std::max(threadCount + 1, 2U);
    _mainTaskPool.size_controller().resize(_workerThreadCount);

    return true;
}

void TaskPool::flushCallbackQueue()
{
    U32 taskIndex = 0;
    while (_threadedCallbackBuffer.pop(taskIndex)) {
        const DELEGATE_CBK<>& cbk = _taskCallbacks[taskIndex];
        if (cbk) {
            cbk();
            _taskStates[taskIndex] = false;
            _taskCallbacks[taskIndex].swap(DELEGATE_CBK<>());
        }
    }
}

void TaskPool::waitForAllTasks(bool yeld, bool flushCallbacks) {
    bool finished = false;
    while (!finished) {
        finished = true;
        for (bool state : _taskStates) {
            if (state) {
                finished = false;
                break;
            }
        }
        if (yeld) {
            std::this_thread::yield();
        }
    }
    if (flushCallbacks) {
        flushCallbackQueue();
    }
}

void TaskPool::setTaskCallback(const TaskHandle& handle,
                               const DELEGATE_CBK<>& callback) {
    U32 index = handle._task->poolIndex();
    assert(!_taskCallbacks[index]);

    _taskCallbacks[index] = callback;
}

void TaskPool::taskCompleted(U32 poolIndex) {
    if (!_taskCallbacks[poolIndex]) {
        _taskStates[poolIndex] = false;
    } else {
        WAIT_FOR_CONDITION(_threadedCallbackBuffer.push(poolIndex));
    }
    // Signal main thread to execute callback
}

TaskHandle TaskPool::getTaskHandle(I64 taskGUID) {
    for (Task& freeTask : _tasksPool) {
        if (freeTask.getGUID() == taskGUID) {
            return TaskHandle(&freeTask, freeTask.jobIdentifier());
        }
    }
    // return the first task instead of a dummy result
    return _tasksPool.empty() ? TaskHandle(nullptr, -1) 
                              : TaskHandle(&_tasksPool.front(), -1);
}

Task& TaskPool::getAvailableTask() {
    const U32 poolSize = to_const_uint(_tasksPool.size());

    U32 taskIndex = (++_allocatedJobs - 1u) & (poolSize - 1u);
    U32 failCount = 0;
    while(_taskStates[taskIndex]) {
        failCount++;
        taskIndex = (++_allocatedJobs - 1u) & (poolSize - 1u);
        assert(failCount < poolSize * 2);
    }

    Task& task = _tasksPool[taskIndex];
    task.reset();

    return task;
}

TaskHandle GetTaskHandle(I64 taskGUID) {
    return GetTaskHandle(Application::instance()
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
    TaskPool& pool = Application::instance().kernel().taskPool();
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

void WaitForAllTasks(bool yeld, bool flushCallbacks) {
    TaskPool& pool = Application::instance().kernel().taskPool();
    WaitForAllTasks(pool, yeld, flushCallbacks);
}

void WaitForAllTasks(TaskPool& pool, bool yeld, bool flushCallbacks) {
    pool.waitForAllTasks(yeld, flushCallbacks);
}

TaskHandle parallel_for(const DELEGATE_CBK_PARAM_3<const std::atomic_bool&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority,
                        bool waitForResult)
{
    TaskPool& pool = Application::instance().kernel().taskPool();
    return parallel_for(pool, cbk, count, partitionSize, priority, waitForResult);
}

TaskHandle parallel_for(TaskPool& pool, 
                        const DELEGATE_CBK_PARAM_3<const std::atomic_bool&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority,
                        bool waitForResult)
{
    U32 crtPartitionSize = std::min(partitionSize, count);
    U32 partitionCount = count / crtPartitionSize;
    U32 remainder = count % crtPartitionSize;

    TaskHandle updateTask = CreateTask(pool, DELEGATE_CBK_PARAM<bool>());
    for (U32 i = 0; i < partitionCount; ++i) {
        U32 start = i * crtPartitionSize;
        U32 end = start + crtPartitionSize;
        updateTask.addChildTask(CreateTask(pool,
                                           DELEGATE_BIND(cbk,
                                                         std::placeholders::_1,
                                                         start,
                                                         end))._task)->startTask(priority);
    }
    if (remainder > 0) {
        updateTask.addChildTask(CreateTask(pool,
                                           DELEGATE_BIND(cbk,
                                                         std::placeholders::_1,
                                                         count - remainder,
                                                         count))._task)->startTask(priority);
    }

    updateTask.startTask(priority);
    if (waitForResult) {
        updateTask.wait();
    }

    return updateTask;
}
};
