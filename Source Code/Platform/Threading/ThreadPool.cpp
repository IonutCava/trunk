#include "stdafx.h"

#include "Headers/ThreadPool.h"

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

    BlockingThreadPool::~BlockingThreadPool()
    {
    }

    void BlockingThreadPool::addTask(const PoolTask& job)  {
        _queue.enqueue(job);
        _tasksLeft.fetch_add(1);
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

    LockFreeThreadPool::~LockFreeThreadPool()
    {
    }

    void LockFreeThreadPool::addTask(const PoolTask& job) {
        _queue.enqueue(job);
        _tasksLeft.fetch_add(1);
    }
   
};