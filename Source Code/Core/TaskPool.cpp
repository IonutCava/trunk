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
      _stopRequested(false)
{
    _threadCount = 0u;
}

TaskPool::~TaskPool()
{
    shutdown();
}

bool TaskPool::init(U32 threadCount, TaskPoolType poolType, const DELEGATE<void, const std::thread::id&>& onThreadCreate, const stringImpl& workerName) {
    if (threadCount == 0 || _poolImpl.init()) {
        return false;
    }

    _threadNamePrefix = workerName;
    _threadCreateCbk = onThreadCreate;
    _workerThreadCount = threadCount;

    switch (poolType) {
        case TaskPoolType::TYPE_LOCKFREE: {
            _poolImpl._poolImplLockFree = std::make_unique<ThreadPool<false>>(*this, _workerThreadCount);
            _poolImpl._isBlocking = false;
        } break;
        case TaskPoolType::TYPE_BLOCKING: {
            _poolImpl._poolImplBlocking = std::make_unique<ThreadPool<true>>(*this, _workerThreadCount);
            _poolImpl._isBlocking = true;
        }break;
    }

    _stopRequested.store(false);

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true, true);
}

void TaskPool::onThreadCreate(const std::thread::id& threadID) {
    const stringImpl threadName = _threadNamePrefix + to_stringImpl(_threadCount.fetch_add(1));
    if (USE_OPTICK_PROFILER) {
        OPTICK_START_THREAD(threadName.c_str());
    }

    setThreadName(threadName.c_str());
    if (_threadCreateCbk) {
        _threadCreateCbk(threadID);
    }
}

void TaskPool::onThreadDestroy(const std::thread::id& threadID) {
    if (USE_OPTICK_PROFILER) {
        OPTICK_STOP_THREAD();
    }
}

bool TaskPool::enqueue(PoolTask&& task, TaskPriority priority, U32 taskIndex, DELEGATE<void>&& onCompletionFunction) {
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

    return _poolImpl.addTask(std::move(task));
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
    if (USE_OPTICK_PROFILER) {
        OPTICK_EVENT();
    }

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

    bool finished = _workerThreadCount == 0u;
    while (!finished) {
        finished = _runningTaskCount.load() == 0u;
        if (!finished && yield) {
            std::this_thread::yield();
        }
    }

    if (flushCallbacks) {
        flushCallbackQueue();
    }

    _stopRequested.store(false);
    _poolImpl.waitAndJoin();
}

void TaskPool::taskCompleted(U32 taskIndex, TaskPriority priority, bool hasOnCompletionFunction) {
    if (hasOnCompletionFunction) {
        _threadedCallbackBuffer.enqueue(taskIndex);
    }

    _runningTaskCount.fetch_sub(1);
}

Task* TaskPool::createTask(Task* parentTask, const DELEGATE<void, Task&>& threadedFunction, bool allowedInIdle)
{
    if (parentTask != nullptr) {
        parentTask->_unfinishedJobs.fetch_add(1);
    }

    Task* task = nullptr;
    do {
        Task& crtTask = g_taskAllocator[g_allocatedTasks++ & (Config::MAX_POOLED_TASKS - 1u)];

        U16 expected = to_U16(0u);
        if (crtTask._unfinishedJobs.compare_exchange_strong(expected, 1u)) {
            task = &crtTask;
        }
    } while (task == nullptr);

    task->_parent = parentTask;
    task->_parentPool = this;
    task->_callback = threadedFunction;
    task->_runWhileIdle = allowedInIdle;

    if (task->_id == 0) {
        task->_id = g_taskIDCounter.fetch_add(1u);
    }

    return task;
}

bool TaskPool::stopRequested() const noexcept {
    return _stopRequested.load();
}

void TaskPool::threadWaiting() {
    _poolImpl.threadWaiting();
}

Task* CreateTask(TaskPool& pool, const DELEGATE<void, Task&>& threadedFunction, bool allowedInIdle)
{
    return CreateTask(pool, nullptr, threadedFunction, allowedInIdle);
}

Task* CreateTask(TaskPool& pool, Task* parentTask, const DELEGATE<void, Task&>& threadedFunction, bool allowedInIdle)
{
    return pool.createTask(parentTask, threadedFunction, allowedInIdle);
}

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks, bool foceClear) {
    pool.waitForAllTasks(yield, flushCallbacks, foceClear);
}

void parallel_for(TaskPool& pool, 
                  const DELEGATE<void, Task*, U32, U32>& cbk,
                  const ParallelForDescriptor& descriptor)
{
    if (descriptor._iterCount > 0) {
        const U32 crtPartitionSize = std::min(descriptor._partitionSize, descriptor._iterCount);
        const U32 partitionCount = descriptor._iterCount / crtPartitionSize;
        const U32 remainder = descriptor._iterCount % crtPartitionSize;
        const U32 adjustedCount = descriptor._useCurrentThread ? partitionCount - 1 : partitionCount;

        std::atomic_uint jobCount = adjustedCount + (remainder > 0 ? 1 : 0);

        for (U32 i = 0; i < adjustedCount; ++i) {
            const U32 start = i * crtPartitionSize;
            const U32 end = start + crtPartitionSize;
            Task* parallel_job = CreateTask(pool,
                                            nullptr,
                                            [&cbk, &jobCount, start, end](Task& parentTask) {
                                                if (TaskPool::USE_OPTICK_PROFILER) {
                                                    OPTICK_EVENT();
                                                }
                                                cbk(&parentTask, start, end);
                                                jobCount.fetch_sub(1);
                                            }, descriptor._allowRunInIdle);
  
            Start(*parallel_job, descriptor._priority);
        }
        if (remainder > 0) {
            const U32 count = descriptor._iterCount;
            Task* parallel_job = CreateTask(pool,
                                            nullptr,
                                            [&cbk, &jobCount, count, remainder](Task& parentTask) {
                                                if (TaskPool::USE_OPTICK_PROFILER) {
                                                    OPTICK_EVENT();
                                                }
                                                cbk(&parentTask, count - remainder, count);
                                                jobCount.fetch_sub(1);
                                            });

            Start(*parallel_job, descriptor._priority);
        }

        if (descriptor._useCurrentThread) {
            const U32 start = adjustedCount * crtPartitionSize;
            const U32 end = start + crtPartitionSize;
            if (TaskPool::USE_OPTICK_PROFILER) {
                OPTICK_EVENT();
            }
            cbk(nullptr, start, end);
        }

        if (descriptor._waitForFinish) {
            if (descriptor._allowPoolIdle) {
                while (jobCount.load() > 0) {
                    pool.threadWaiting();
                }
            } else {
                WAIT_FOR_CONDITION(jobCount.load() == 0u);
            }
        }
    }
}

bool TaskPool::PoolHolder::init() const noexcept {
    return _poolImplBlocking != nullptr || _poolImplLockFree != nullptr;
}

void TaskPool::PoolHolder::waitAndJoin() {
    if (_isBlocking) {
        _poolImplBlocking->wait();
        _poolImplBlocking->join();
    } else {
        _poolImplLockFree->wait();
        _poolImplLockFree->join();
    }
}

void TaskPool::PoolHolder::threadWaiting() {
    if (_isBlocking) {
        _poolImplBlocking->executeOneTask(false);
    } else {
        _poolImplLockFree->executeOneTask(false);
    }
}

bool TaskPool::PoolHolder::addTask(PoolTask&& job) {
    if (_isBlocking) {
        return _poolImplBlocking->addTask(std::move(job));
    }

    return _poolImplLockFree->addTask(std::move(job));
}
};
