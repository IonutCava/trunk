#include "stdafx.h"

#include "Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

TaskPool::TaskPool(U32 maxTaskCount)
    : _threadedCallbackBuffer(maxTaskCount),
      _workerThreadCount(0u),
      _tasksPool(maxTaskCount),
      _taskCallbacks(maxTaskCount),
      _taskStates(maxTaskCount, TaskState::TASK_FREE)
{
    assert(isPowerOfTwo(maxTaskCount));
    _allocatedJobs.store(0);
}

TaskPool::~TaskPool()
{
    shutdown();
}

bool TaskPool::init(U32 threadCount, TaskPoolType type, const stringImpl& workerName) {
    if (threadCount == 0 || _mainTaskPool != nullptr) {
        return false;
    }

    _workerThreadCount = threadCount;

    switch (type) {
        case TaskPoolType::PRIORITY_QUEUE:
            _mainTaskPool = std::make_unique<ThreadPoolBoostPrio>(_workerThreadCount);
            break;
        case TaskPoolType::FIFO_QUEUE:
            _mainTaskPool = std::make_unique<ThreadPoolBoostFifo>(_workerThreadCount);
            break;
        case TaskPoolType::DONT_CARE:
        default:
            _mainTaskPool = std::make_unique<ThreadPoolC11>(_workerThreadCount);
            break;
    };

    nameThreadpoolWorkers(workerName.c_str(), *_mainTaskPool);

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true, true);
    _tasksPool.clear();
}
void TaskPool::runCbkAndClearTask(size_t taskIndex) {
    DELEGATE_CBK<void>& cbk = _taskCallbacks[taskIndex];
    if (cbk) {
        cbk();
        cbk = DELEGATE_CBK<void>();
    }
}

TaskPool::TaskState TaskPool::state(size_t index) const {
    ReadLock lk(_taskStateLock);
    return _taskStates[index];
}

void TaskPool::flushCallbackQueue()
{
    size_t taskIndex = 0;
    while (_threadedCallbackBuffer.pop(taskIndex)) {
        runCbkAndClearTask(taskIndex);
    }
}

void TaskPool::waitForAllTasks(bool yield, bool flushCallbacks, bool forceClear) {
    bool finished = _workerThreadCount == 0;
    while (!finished) {
        if (forceClear) {
            std::for_each(std::begin(_tasksPool),
                          std::end(_tasksPool),
                          [](Task& task) {
                              task.stopTask();
                          });
        }
        // Possible race condition. Try to set all child states to false as well!
        ReadLock lk(_taskStateLock);
        finished = std::find_if(std::cbegin(_taskStates),
                                std::cend(_taskStates),
                                [](TaskState entry) {
                                    return entry != TaskState::TASK_FREE;
                                }) == std::cend(_taskStates);
        if (yield) {
            std::this_thread::yield();
        }
    }
    if (flushCallbacks) {
        flushCallbackQueue();
    }

    _mainTaskPool->stopAll();
    _mainTaskPool->waitAll();
}

void TaskPool::taskStarted(size_t poolIndex) {
    WriteLock lk(_taskStateLock);
    _taskStates[poolIndex] = TaskState::TASK_RUNNING;
}

void TaskPool::taskCompleted(size_t poolIndex, bool runCallback) {
    if (runCallback) {
        runCbkAndClearTask(poolIndex);
    } else {
        WAIT_FOR_CONDITION(_threadedCallbackBuffer.push(poolIndex));
    }
    WriteLock lk(_taskStateLock);
    _taskStates[poolIndex] = TaskState::TASK_FREE;
}

Task* TaskPool::createTask(Task* parentTask,
                           I64 jobIdentifier,
                           const DELEGATE_CBK<void, const Task&>& threadedFunction,
                           const DELEGATE_CBK<void>& onCompletionFunction) 
{
    TaskDescriptor descriptor;
    descriptor.parentTask = parentTask;
    descriptor.cbk = threadedFunction;

    const size_t poolSize = to_base(_tasksPool.size());

    size_t taskIndex = _allocatedJobs.fetch_add(1u) & (poolSize - 1u);
    size_t failCount = 0;

    {
        WriteLock lk(_taskStateLock);
        while(_taskStates[taskIndex] != TaskState::TASK_FREE) {
            failCount++;
            taskIndex = _allocatedJobs.fetch_add(1u) & (poolSize - 1u);
            assert(failCount < poolSize * 2);
        }
        _taskStates[taskIndex] = TaskState::TASK_ALLOCATED;
    }

    _taskCallbacks[taskIndex] = onCompletionFunction;
    descriptor.index = taskIndex;
    return new(&_tasksPool[taskIndex]) Task{ descriptor };
}

//Ref: https://gist.github.com/shotamatsuda/e11ed41ee2978fa5a2f1/
void TaskPool::nameThreadpoolWorkers(const char* name, ThreadPool& pool) {
    std::mutex mutex;
    std::condition_variable condition;
    bool predicate = false;
    UniqueLock lock(mutex);
    for (std::size_t i = 0; i < pool.numThreads(); ++i) {
        pool.enqueue(PoolTask(1, [i, &name, &mutex, &condition, &predicate]() {
            UniqueLock lock(mutex);
            while (!predicate) {
                condition.wait(lock);
            }
            setThreadName(Util::StringFormat("%s_%d", name, i).c_str());
        }));
    }
    predicate = true;
    condition.notify_all();
    lock.unlock();
    pool.waitAll();
}

TaskHandle CreateTask(TaskPool& pool,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction)
{
    return CreateTask(pool, -1, threadedFunction, onCompletionFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction)
{
    return CreateTask(pool, parentTask , -1, threadedFunction, onCompletionFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                      I64 jobIdentifier,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction)
{
    return CreateTask(pool, nullptr, jobIdentifier, threadedFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      I64 jobIdentifier,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction,
                      const DELEGATE_CBK<void>& onCompletionFunction)
{
    Task* freeTask = pool.createTask(parentTask ? parentTask->_task : nullptr,
                                     jobIdentifier,
                                     threadedFunction,
                                     onCompletionFunction);
    return TaskHandle(freeTask, &pool, jobIdentifier);
}

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks, bool foceClear) {
    pool.waitForAllTasks(yield, flushCallbacks, foceClear);
}


TaskHandle parallel_for(TaskPool& pool, 
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority,
                        U32 taskFlags)
{
    TaskHandle updateTask = CreateTask(pool, DELEGATE_CBK<void, const Task&>());
    if (count > 0) {

        U32 crtPartitionSize = std::min(partitionSize, count);
        U32 partitionCount = count / crtPartitionSize;
        U32 remainder = count % crtPartitionSize;

        for (U32 i = 0; i < partitionCount; ++i) {
            U32 start = i * crtPartitionSize;
            U32 end = start + crtPartitionSize;
            CreateTask(pool,
                       &updateTask,
                       [&cbk, start, end](const Task& parentTask) {
                           cbk(parentTask, start, end);
                       }).startTask(priority, taskFlags);
        }
        if (remainder > 0) {
            CreateTask(pool,
                       &updateTask,
                       [&cbk, count, remainder](const Task& parentTask) {
                           cbk(parentTask, count - remainder, count);
                       }).startTask(priority, taskFlags);
        }
    }

    updateTask.startTask(priority, taskFlags).wait();

    return updateTask;
}

};
