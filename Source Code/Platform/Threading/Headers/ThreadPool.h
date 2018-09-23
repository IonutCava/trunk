/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _PLATFORM_TASK_POOL_H_
#define _PLATFORM_TASK_POOL_H_

namespace Divide {

typedef std::function<void()> PoolTask;

// Dead simple ThreadPool class
class ThreadPool
{
public:

    explicit ThreadPool(const U8 threadCount);
    virtual ~ThreadPool();

    // Add a new task to the pool's queue
    virtual bool addTask(const PoolTask& job) = 0;

    // Join all of the threads and block until all running tasks have completed.
    void join();

    // Wait for all running jobs to finish
    virtual void wait();

    // Get all of the threads for external usage (e.g. setting affinity)
    eastl::vector<std::thread>& threads();

protected:
    bool _isRunning = false;;
    std::atomic_int _tasksLeft;
    eastl::vector<std::thread> _threads;
};

class BoostAsioThreadPool final : public ThreadPool
{
public:

    explicit BoostAsioThreadPool(const U8 threadCount);
    ~BoostAsioThreadPool();

    // Add a new task to the pool's queue
    bool addTask(const PoolTask& job) override;
    void wait();

private:
    boost::asio::thread_pool* _queue;
};

class BlockingThreadPool final : public ThreadPool
{
public:

    explicit BlockingThreadPool(const U8 threadCount);
    ~BlockingThreadPool() = default;

    // Add a new task to the pool's queue
    bool addTask(const PoolTask& job) override;

private:
    moodycamel::BlockingConcurrentQueue<PoolTask> _queue;
};


class LockFreeThreadPool final : public ThreadPool
{
public:

    explicit LockFreeThreadPool(const U8 threadCount);
    ~LockFreeThreadPool() = default;

    // Add a new task to the pool's queue
    bool addTask(const PoolTask& job) override;

private:
    moodycamel::ConcurrentQueue<PoolTask> _queue;
};
}; //namespace Divide

#endif //_PLATFORM_TASK_POOL_H_