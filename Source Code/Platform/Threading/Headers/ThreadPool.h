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

using PoolTask = std::function<bool(bool)>;

class TaskPool;

// Dead simple ThreadPool class
class ThreadPool
{
public:

    explicit ThreadPool(TaskPool& parent, const U32 threadCount);
    virtual ~ThreadPool();

    // Add a new task to the pool's queue
    virtual bool addTask(PoolTask&& job) = 0;

    // Join all of the threads and block until all running tasks have completed.
    void join();

    // Wait for all running jobs to finish
    void wait() noexcept;

    // Get all of the threads for external usage (e.g. setting affinity)
    eastl::vector<std::thread>& threads() noexcept;

    virtual void executeOneTask(bool waitForTask) = 0;

protected:
    void onThreadCreate(const std::thread::id& threadID);
    void onThreadDestroy(const std::thread::id& threadID);

protected:
    TaskPool& _parent;
    bool _isRunning = false;
    std::atomic_int _tasksLeft;
    eastl::vector<std::thread> _threads;
};

class BlockingThreadPool final : public ThreadPool
{
public:

    explicit BlockingThreadPool(TaskPool& parent, const U32 threadCount);
    ~BlockingThreadPool() = default;

    // Add a new task to the pool's queue
    bool addTask(PoolTask&& job) override;

    void executeOneTask(bool waitForTask) override;

private:
    moodycamel::BlockingConcurrentQueue<PoolTask> _queue;
};

class LockFreeThreadPool final : public ThreadPool
{
public:

    explicit LockFreeThreadPool(TaskPool& parent, const U32 threadCount);
    ~LockFreeThreadPool() = default;

    // Add a new task to the pool's queue
    bool addTask(PoolTask&& job) override;

    void executeOneTask(bool waitForTask) override;

private:
    moodycamel::ConcurrentQueue<PoolTask> _queue;
};
}; //namespace Divide

#endif //_PLATFORM_TASK_POOL_H_