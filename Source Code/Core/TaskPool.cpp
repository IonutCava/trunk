#include "stdafx.h"

#include "Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

namespace {
    constexpr bool g_forceSingleThreaded = false;

    thread_local Task g_taskAllocator[Config::MAX_POOLED_TASKS];
    thread_local U32  g_allocatedTasks = 0u;
};

TaskPool::TaskPool() noexcept
    : GUIDWrapper(),
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

bool TaskPool::init(U8 threadCount, TaskPoolType poolType, const stringImpl& workerName) {
    if (threadCount == 0 || _mainTaskPool != nullptr) {
        return false;
    }

    _workerThreadCount = threadCount;
    switch (poolType) {
        case TaskPoolType::TYPE_BOOST_ASIO: {
            _mainTaskPool = std::make_unique<BoostAsioThreadPool>(_workerThreadCount);
        } break;
        case TaskPoolType::TYPE_LOCKFREE: {
            _mainTaskPool = std::make_unique<LockFreeThreadPool>(_workerThreadCount);
        } break;
        case TaskPoolType::TYPE_BLOCKING: {
            _mainTaskPool = std::make_unique<BlockingThreadPool>(_workerThreadCount);
        }break;
    }

    _stopRequested.store(false);
    nameThreadpoolWorkers(workerName.c_str());

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true, true);
}

bool TaskPool::enqueue(const PoolTask& task, TaskPriority priority) {
    _runningTaskCount.fetch_add(1);

    if (!g_forceSingleThreaded && priority != TaskPriority::REALTIME) {
        assert(_mainTaskPool != nullptr);
        if (!_mainTaskPool->addTask(task)) {
            return false;
        }
    } else {
        task();
    }

    return true;
}

void TaskPool::runCbkAndClearTask(U32 taskIdentifier) {
    DELEGATE_CBK<void>& cbk = _taskCallbacks[taskIdentifier];
    if (cbk) {
        cbk();
        cbk = 0;
    }
}


void TaskPool::flushCallbackQueue() {
    U32 taskIndex = 0;
    while (_threadedCallbackBuffer.try_dequeue(taskIndex)) {
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
    _mainTaskPool->wait();
    _mainTaskPool->join();
}

void TaskPool::taskCompleted(U32 taskIndex) {
    _runningTaskCount.fetch_sub(1);
}

void TaskPool::taskCompleted(U32 taskIndex, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction) {
    if (onCompletionFunction) {
        if (g_forceSingleThreaded || priority == TaskPriority::REALTIME) {
            onCompletionFunction();
        } else {
            _taskCallbacks[taskIndex] = onCompletionFunction;
            _threadedCallbackBuffer.enqueue(taskIndex);
        }
    }

    taskCompleted(taskIndex);
}

Task* TaskPool::createTask(Task* parentTask, const DELEGATE_CBK<void, const Task&>& threadedFunction) 
{
    if (parentTask != nullptr) {
        parentTask->_unfinishedJobs.fetch_add(1);
    }

    Task* task = nullptr;
    while (task == nullptr) {
        const U32 index = g_allocatedTasks++;
        Task& crtTask = g_taskAllocator[index & (Config::MAX_POOLED_TASKS - 1u)];
        if (Finished(crtTask)) {
            task = &crtTask;
        }
    }

    task->_parentPool = this;
    task->_parent = parentTask;
    task->_callback = threadedFunction;
    task->_unfinishedJobs = 1;

    if (task->_id == 0) {
        static U32 id = 1;
        task->_id = id++;
    }

    return task;
}

//Ref: https://gist.github.com/shotamatsuda/e11ed41ee2978fa5a2f1/
void TaskPool::nameThreadpoolWorkers(const char* name) {
    std::mutex mutex;
    std::condition_variable condition;
    bool predicate = false;
    std::atomic<U8> count = 0;
    UniqueLock lock(mutex);
    for (U8 i = 0; i < _workerThreadCount; ++i) {
        enqueue([i, &name, &mutex, &condition, &predicate, &count]() {
            UniqueLock lock(mutex);
            while (!predicate) {
                condition.wait(lock);
            }
            setThreadName(Util::StringFormat("%s_%d", name, i).c_str());
            ++count;
        }, TaskPriority::DONT_CARE);
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

TaskHandle CreateTask(TaskPool& pool, const DELEGATE_CBK<void, const Task&>& threadedFunction)
{
    return CreateTask(pool, nullptr, threadedFunction);
}

TaskHandle CreateTask(TaskPool& pool,
                      TaskHandle* parentTask,
                      const DELEGATE_CBK<void, const Task&>& threadedFunction)
{
    Task* freeTask = pool.createTask(parentTask ? parentTask->_task : nullptr,
                                     threadedFunction);
    return TaskHandle(freeTask, &pool);
}

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks, bool foceClear) {
    pool.waitForAllTasks(yield, flushCallbacks, foceClear);
}

TaskHandle parallel_for(TaskPool& pool, 
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        TaskPriority priority)
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
                       }).startTask(priority);
        }
        if (remainder > 0) {
            CreateTask(pool,
                       &updateTask,
                       [&cbk, count, remainder](const Task& parentTask) {
                           cbk(parentTask, count - remainder, count);
                       }).startTask(priority);
        }
    }

    updateTask.startTask(Runtime::isMainThread() ? priority : TaskPriority::REALTIME).wait();

    return updateTask;
}

};
