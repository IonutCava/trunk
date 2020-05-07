#include "stdafx.h"

#include "Platform/Threading/Headers/Task.h"
#include "Headers/TaskPool.h"
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
      _workerThreadCount(0u)
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
            std::get<1>(_poolImpl._poolImpl) = std::make_unique<ThreadPool<false>>(*this, _workerThreadCount);
        } break;
        case TaskPoolType::TYPE_BLOCKING: {
            std::get<0>(_poolImpl._poolImpl) = std::make_unique<ThreadPool<true>>(*this, _workerThreadCount);
        } break;
    }

    return true;
}

void TaskPool::shutdown() {
    waitForAllTasks(true, true);
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
        WAIT_FOR_CONDITION(task(false));
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

void TaskPool::waitForAllTasks(bool yield, bool flushCallbacks) {
    if (!_poolImpl.init()) {
        return;
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

    _poolImpl.waitAndJoin();
}

void TaskPool::taskCompleted(U32 taskIndex, bool hasOnCompletionFunction) {
    if (hasOnCompletionFunction) {
        _threadedCallbackBuffer.enqueue(taskIndex);
    }

    _runningTaskCount.fetch_sub(1);
}

Task* TaskPool::allocateTask(Task* parentTask, bool allowedInIdle) {
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
    task->_runWhileIdle = allowedInIdle;
    if (task->_parent != nullptr) {
        task->_parent->_unfinishedJobs.fetch_add(1);
    }
    if (task->_id == 0) {
        task->_id = g_taskIDCounter.fetch_add(1u);
    }

    return task;
}

void TaskPool::threadWaiting() {
    _poolImpl.threadWaiting();
}

void WaitForAllTasks(TaskPool& pool, bool yield, bool flushCallbacks) {
    pool.waitForAllTasks(yield, flushCallbacks);
}

bool TaskPool::PoolHolder::init() const noexcept {
    return _poolImpl.first != nullptr || 
           _poolImpl.second != nullptr;
}

void TaskPool::PoolHolder::waitAndJoin() {
    if (_poolImpl.first != nullptr) {
        _poolImpl.first->wait();
        _poolImpl.first->join();
        return;
    }

    _poolImpl.second->wait();
    _poolImpl.second->join();
}

void TaskPool::PoolHolder::threadWaiting() {
    if (_poolImpl.first != nullptr) {
        _poolImpl.first->executeOneTask(false);
        return;
    }
    _poolImpl.second->executeOneTask(false);
}

bool TaskPool::PoolHolder::addTask(PoolTask&& job) {
    if (_poolImpl.first != nullptr) {
        return _poolImpl.first->addTask(std::move(job));
    }

    return _poolImpl.second->addTask(std::move(job));
}
};
