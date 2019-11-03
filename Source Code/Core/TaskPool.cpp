#include "stdafx.h"

#include "Headers/TaskPool.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

namespace {
    std::atomic_uint g_taskIDCounter = 0u;
    thread_local Task g_taskAllocator[Config::MAX_POOLED_TASKS];
    thread_local U64  g_allocatedTasks = 0u;
};

TaskPool::TaskPool()
    : GUIDWrapper(),
      _taskCallbacks(Config::MAX_POOLED_TASKS),
      _runningTaskCount(0u),
      _workerThreadCount(0u),
      _stopRequested(false),
      _poolImpl(nullptr)
{
    _threadCount = 0u;
}

TaskPool::~TaskPool()
{
    shutdown();
}

bool TaskPool::init(U8 threadCount, TaskPoolType poolType, const DELEGATE_CBK<void, const std::thread::id&>& onThreadCreate, const stringImpl& workerName) {
    if (threadCount == 0 || _poolImpl != nullptr) {
        return false;
    }
    _threadNamePrefix = workerName;
    _threadCreateCbk = onThreadCreate;
    _workerThreadCount = threadCount;

    switch (poolType) {
        case TaskPoolType::TYPE_LOCKFREE: {
            _poolImpl = std::make_unique<LockFreeThreadPool>(*this, _workerThreadCount);
        } break;
        case TaskPoolType::TYPE_BLOCKING: {
            _poolImpl = std::make_unique<BlockingThreadPool>(*this, _workerThreadCount);
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

bool TaskPool::enqueue(PoolTask&& task, TaskPriority priority, U32 taskIndex, DELEGATE_CBK<void>&& onCompletionFunction) {
    _runningTaskCount.fetch_add(1);

    if (priority == TaskPriority::REALTIME) {
        WAIT_FOR_CONDITION(task(true));
        if (onCompletionFunction) {
            onCompletionFunction();
        }
        return true;
    } else if (onCompletionFunction) {
        _taskCallbacks[taskIndex].push_back(onCompletionFunction);
    }

    return _poolImpl->addTask(std::move(task));
}

void TaskPool::runCbkAndClearTask(U32 taskIdentifier) {
    auto& cbks = _taskCallbacks[taskIdentifier];
    for (auto& cbk : cbks) {
        if (cbk) {
            cbk();
            cbk = 0;
        }
    }
    cbks.resize(0);
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
    _poolImpl->wait();
    _poolImpl->join();
}

void TaskPool::taskCompleted(U32 taskIndex, TaskPriority priority, bool hasOnCompletionFunction) {
    if (hasOnCompletionFunction) {
        _threadedCallbackBuffer.enqueue(taskIndex);
    }

    _runningTaskCount.fetch_sub(1);
}

Task* TaskPool::createTask(Task* parentTask, const DELEGATE_CBK<void, Task&>& threadedFunction, const char* debugName) 
{
    if (parentTask != nullptr) {
        parentTask->_unfinishedJobs.fetch_add(1, std::memory_order_relaxed);
    }

    Task* task = nullptr;
    do {
        constexpr U16 target = to_U16(1u); 
        U16 expected = to_U16(0u);

        Task& crtTask = g_taskAllocator[g_allocatedTasks++ & (Config::MAX_POOLED_TASKS - 1u)];
        if (crtTask._unfinishedJobs.compare_exchange_weak(expected, target, std::memory_order_acquire, std::memory_order_relaxed)) {
            task = &crtTask;
        }
    } while (task == nullptr);

    task->_parent = parentTask;
    task->_parentPool = this;
    task->_callback = threadedFunction;
    if (task->_id == 0) {
        task->_id = g_taskIDCounter.fetch_add(1u);
    }

    return task;
}

bool TaskPool::stopRequested() const noexcept {
    return _stopRequested.load();
}

void TaskPool::threadWaiting() {
    _poolImpl->executeOneTask(false);
}

Task* CreateTask(TaskPool& pool, const DELEGATE_CBK<void, Task&>& threadedFunction, const char* debugName)
{
    return CreateTask(pool, nullptr, threadedFunction, debugName);
}

Task* CreateTask(TaskPool& pool, Task* parentTask, const DELEGATE_CBK<void, Task&>& threadedFunction, const char* debugName)
{
    return pool.createTask(parentTask, threadedFunction, debugName);
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
                  bool useCurrentThread,
                  const char* debugName)
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
            Start(*CreateTask(pool,
                       nullptr,
                       [&cbk, &jobCount, start, end](const Task& parentTask) {
                           cbk(parentTask, start, end);
                           jobCount.fetch_sub(1);
                       },
                debugName), priority);
        }
        if (remainder > 0) {
            Start(*CreateTask(pool,
                       nullptr,
                       [&cbk, &jobCount, count, remainder](const Task& parentTask) {
                           cbk(parentTask, count - remainder, count);
                           jobCount.fetch_sub(1);
                       },
                debugName), priority);
        }

        if (useCurrentThread) {
            Task* threadTask = CreateTask(pool, [](const Task& parentTask) {ACKNOWLEDGE_UNUSED(parentTask); }, debugName);
            const U32 start = adjustedCount * crtPartitionSize;
            const U32 end = start + crtPartitionSize;
            cbk(*threadTask, start, end);
            threadTask->_unfinishedJobs.fetch_sub(1);
        }
        if (!noWait) {
            while (jobCount.load() > 0) {
                if (!Runtime::isMainThread()) {
                    pool.threadWaiting();
                }
            }
        }
    }
}

};
