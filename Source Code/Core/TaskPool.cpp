#include "stdafx.h"

#include "Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

namespace {
    thread_local Task g_taskAllocator[Config::MAX_POOLED_TASKS];
    thread_local U32  g_allocatedTasks = 0u;
};

TaskPool::TaskPool()
    : _threadedCallbackBuffer(Config::MAX_POOLED_TASKS),
      _taskCallbacks(Config::MAX_POOLED_TASKS),
      _runningTaskCount(0u),
      _workerThreadCount(0u),
      _stopRequested(false)
{
}

TaskPool::~TaskPool()
{
    shutdown();
}

bool TaskPool::init(U32 threadCount, const stringImpl& workerName) {
    if (threadCount == 0 || _mainTaskPool != nullptr) {
        return false;
    }

    _workerThreadCount = threadCount;
    _mainTaskPool = std::make_unique<boost::asio::thread_pool>(_workerThreadCount);
    _stopRequested.store(false);
    nameThreadpoolWorkers(workerName.c_str());

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true, true);
}

bool TaskPool::enqueue(const PoolTask& task) {
    assert(_mainTaskPool != nullptr);
    boost::asio::post(*_mainTaskPool, task);
    _runningTaskCount.fetch_add(1);

    return true;
}

void TaskPool::runCbkAndClearTask(size_t taskIdentifier) {
    DELEGATE_CBK<void>& cbk = _taskCallbacks[taskIdentifier];
    if (cbk) {
        cbk();
        cbk = DELEGATE_CBK<void>();
    }
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
            _stopRequested.store(true);
        }
        finished = _runningTaskCount.load() == 0;
        if (!finished && yield) {
            std::this_thread::yield();
        }
    }
    if (flushCallbacks) {
        flushCallbackQueue();
    }

    _stopRequested.store(false);
    _mainTaskPool->stop();
    _mainTaskPool->join();
}


void TaskPool::taskCompleted(size_t taskIndex, bool runCallback) {
    if (runCallback) {
        runCbkAndClearTask(taskIndex);
    } else {
        WAIT_FOR_CONDITION(_threadedCallbackBuffer.push(taskIndex));
        _runningTaskCount.fetch_sub(1);
    }
}

Task* TaskPool::createTask(Task* parentTask,
                           I64 jobIdentifier,
                           const DELEGATE_CBK<void, const Task&>& threadedFunction,
                           const DELEGATE_CBK<void>& onCompletionFunction) 
{
    if (parentTask != nullptr) {
        parentTask->_unfinishedJobs.fetch_add(1);
    }

    const U32 index = g_allocatedTasks++;
    Task* task = &g_taskAllocator[index & (Config::MAX_POOLED_TASKS - 1u)];
    task->_parentPool = this;
    task->_parent = parentTask;
    task->_callback = threadedFunction;
    task->_unfinishedJobs = 1;

    if (task->_id == 0) {
        static size_t id = 1;
        task->_id = id++;
    }

    if (onCompletionFunction) {
        _taskCallbacks[task->_id] = onCompletionFunction;
    }

    return task;
}

//Ref: https://gist.github.com/shotamatsuda/e11ed41ee2978fa5a2f1/
void TaskPool::nameThreadpoolWorkers(const char* name) {
    std::mutex mutex;
    std::condition_variable condition;
    bool predicate = false;
    std::atomic_size_t count = 0;
    UniqueLock lock(mutex);
    for (std::size_t i = 0; i < _workerThreadCount; ++i) {
        enqueue([i, &name, &mutex, &condition, &predicate, &count]() {
            UniqueLock lock(mutex);
            while (!predicate) {
                condition.wait(lock);
            }
            setThreadName(Util::StringFormat("%s_%d", name, i).c_str());
            ++count;
        });
    }
    predicate = true;
    condition.notify_all();
    lock.unlock();
    _runningTaskCount.store(0);
    WAIT_FOR_CONDITION(count.load() == _workerThreadCount);
}

bool TaskPool::stopRequested() const {
    return _stopRequested.load();
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
    return CreateTask(pool, nullptr, jobIdentifier, threadedFunction, onCompletionFunction);
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
                        TaskPriority priority,
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
