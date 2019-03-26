#include "stdafx.h"

#include "Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

namespace {
    thread_local Task g_taskAllocator[Config::MAX_POOLED_TASKS];
    thread_local U32  g_allocatedTasks = 0u;
};

TaskPool::TaskPool()
    : GUIDWrapper(),
      _taskCallbacks(Config::MAX_POOLED_TASKS),
      _runningTaskCount(0u),
      _workerThreadCount(0u),
      _stopRequested(false)
{
    _threadCount = 0u;
}

TaskPool::~TaskPool()
{
    shutdown();
}

bool TaskPool::init(U8 threadCount, TaskPoolType poolType, const DELEGATE_CBK<void, const std::thread::id&>& onThreadCreate, const stringImpl& workerName) {
    if (threadCount == 0 || _mainTaskPool != nullptr) {
        return false;
    }
    _threadNamePrefix = workerName;
    _threadCreateCbk = onThreadCreate;
    _workerThreadCount = threadCount;

    switch (poolType) {
        case TaskPoolType::TYPE_LOCKFREE: {
            _mainTaskPool = std::make_unique<LockFreeThreadPool>(*this, _workerThreadCount);
        } break;
        case TaskPoolType::TYPE_BLOCKING: {
            _mainTaskPool = std::make_unique<BlockingThreadPool>(*this, _workerThreadCount);
        }break;
    }

    _stopRequested.store(false);

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true, true);
}

void TaskPool::onThreadCreate(const std::thread::id& threadID) {
    setThreadName((_threadNamePrefix + to_stringImpl(_threadCount.fetch_add(1))).c_str());
    if (_threadCreateCbk) {
        _threadCreateCbk(threadID);
    }
}

bool TaskPool::enqueue(const PoolTask& task, TaskPriority priority) {
    _runningTaskCount.fetch_add(1);

    if (priority == TaskPriority::REALTIME) {
        task();
        return true;
    }

    return _mainTaskPool->addTask(task);
}

void TaskPool::runCbkAndClearTask(U32 taskIdentifier) {
    DELEGATE_CBK<void>& cbk = _taskCallbacks[taskIdentifier];
    if (cbk) {
        cbk();
        cbk = 0;
    }
}

void TaskPool::flushCallbackQueue() {
    constexpr I32 maxDequeueItems = 10;

    U32 taskIndex[maxDequeueItems] = { 0 };
    size_t count = 0;
    do {
        count = _threadedCallbackBuffer.try_dequeue_bulk(taskIndex, maxDequeueItems);
        for (size_t i = 0; i < count; ++i) {
            runCbkAndClearTask(taskIndex[i]);
        }
    } while (count > 0);
}

void TaskPool::waitForAllTasks(bool yield, bool flushCallbacks, bool forceClear) {
    if (forceClear) {
        _stopRequested.store(true);
    }

    bool finished = _workerThreadCount == 0;
    while (!finished) {
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

void TaskPool::taskCompleted(U32 taskIndex, TaskPriority priority, const DELEGATE_CBK<void>& onCompletionFunction) {
    if (onCompletionFunction) {
        if (priority == TaskPriority::REALTIME) {
            onCompletionFunction();
        } else {
            _taskCallbacks[taskIndex] = onCompletionFunction;
            _threadedCallbackBuffer.enqueue(taskIndex);
        }
    }

    _runningTaskCount.fetch_sub(1);
}

Task* TaskPool::createTask(Task* parentTask, const DELEGATE_CBK<void, const Task&>& threadedFunction) 
{
    Task* task = nullptr;
    while (task == nullptr) {
        Task& crtTask = g_taskAllocator[g_allocatedTasks++ & (Config::MAX_POOLED_TASKS - 1u)];
        U16 expected = to_U16(0u);
        if (crtTask._unfinishedJobs.compare_exchange_strong(expected, to_U16(1u))) {
            task = &crtTask;
        }
    }

    if (parentTask != nullptr) {
        parentTask->_unfinishedJobs.fetch_add(1);
    }

    task->_parent = parentTask;
    task->_parentPool = this;
    task->_callback = threadedFunction;

    if (task->_id == 0) {
        static U32 id = 1;
        task->_id = id++;
    }

    return task;
}

bool TaskPool::stopRequested() const noexcept {
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

void parallel_for(TaskPool& pool, 
                  const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                  U32 count,
                  U32 partitionSize,
                  TaskPriority priority,
                  bool noWait,
                  bool useCurrentThread)
{
    if (count > 0) {

        const U32 crtPartitionSize = std::min(partitionSize, count);
        const U32 partitionCount = count / crtPartitionSize;
        const U32 remainder = count % crtPartitionSize;

        U32 adjustedCount = partitionCount;
        if (useCurrentThread) {
            adjustedCount -= 1;
        }

        std::atomic_uint jobCount = adjustedCount + (remainder > 0 ? 1 : 0);

        for (U32 i = 0; i < adjustedCount; ++i) {
            const U32 start = i * crtPartitionSize;
            const U32 end = start + crtPartitionSize;
            CreateTask(pool,
                       nullptr,
                       [&cbk, &jobCount, start, end](const Task& parentTask) {
                           cbk(parentTask, start, end);
                           jobCount.fetch_sub(1);
                       }).startTask(priority);
        }
        if (remainder > 0) {
            CreateTask(pool,
                       nullptr,
                       [&cbk, &jobCount, count, remainder](const Task& parentTask) {
                           cbk(parentTask, count - remainder, count);
                           jobCount.fetch_sub(1);
                       }).startTask(priority);
        }

        if (useCurrentThread) {
            TaskHandle threadTask = CreateTask(pool, [](const Task& parentTask) {ACKNOWLEDGE_UNUSED(parentTask); });
            const U32 start = adjustedCount * crtPartitionSize;
            const U32 end = start + crtPartitionSize;
            cbk(*threadTask._task, start, end);
        }
        if (!noWait) {
            WAIT_FOR_CONDITION(jobCount.load() == 0);
        }
    }
}

};
