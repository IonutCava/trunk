#include "stdafx.h"

#include "Headers/ThreadPool.h"
#include "Platform/Headers/PlatformDefines.h"

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>

namespace Divide {

    ThreadPool::ThreadPool(const U8 threadCount)
        : _isRunning(true)
    {
        _tasksLeft.store(0);

        _threads.reserve(threadCount);
    }

    ThreadPool::~ThreadPool()
    {
        join();
    }

    void ThreadPool::join() {
        if (!_isRunning) {
            return;
        }

        _isRunning = false;

        const vec_size_eastl threadCount = _threads.size();
        for (vec_size_eastl idx = 0; idx < threadCount; ++idx) {
            addTask([] {});
        }

        for (std::thread& thread : _threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void ThreadPool::wait() {
        // Busy wait
        while (_tasksLeft.load() > 0) {
            std::this_thread::yield();
        }
    }

    eastl::vector<std::thread>& ThreadPool::threads() {
        return _threads;
    }

    BoostAsioThreadPool::BoostAsioThreadPool(const U8 threadCount)
        : ThreadPool(threadCount),
          _queue(nullptr)
    {
        _queue = MemoryManager_NEW boost::asio::thread_pool(threadCount);
    }

    BoostAsioThreadPool::~BoostAsioThreadPool()
    {
        MemoryManager::SAFE_DELETE(_queue);
    }

    bool BoostAsioThreadPool::addTask(const PoolTask& job) {
        boost::asio::post(*_queue, job);
        return true;
    }

    void BoostAsioThreadPool::wait() {
        _queue->stop();
        ThreadPool::wait();
    }

    BlockingThreadPool::BlockingThreadPool(const U8 threadCount)
        : ThreadPool(threadCount)
    {
        for (U8 idx = 0; idx < threadCount; ++idx) {
            _threads.push_back(std::thread([&]
            {
                while (_isRunning) {
                    PoolTask task;
                    _queue.wait_dequeue(task);
                    task();

                    _tasksLeft.fetch_sub(1);
                }
            }));
        }
    }

    bool BlockingThreadPool::addTask(const PoolTask& job)  {
        if (_queue.enqueue(job)) {
            _tasksLeft.fetch_add(1);
            return true;
        }

        return false;
    }

    LockFreeThreadPool::LockFreeThreadPool(const U8 threadCount)
        : ThreadPool(threadCount)
    {
        for (U8 idx = 0; idx < threadCount; ++idx) {
            _threads.push_back(std::thread([&]
            {
                while (_isRunning) {
                    PoolTask task;
                    while (!_queue.try_dequeue(task)) {
                        std::this_thread::yield();
                    }
                    task();

                    _tasksLeft.fetch_sub(1);
                }
            }));
        }
    }

    bool LockFreeThreadPool::addTask(const PoolTask& job) {
        if (_queue.enqueue(job)) {
            _tasksLeft.fetch_add(1);
            return true;
        }

        return false;
    }
   
};