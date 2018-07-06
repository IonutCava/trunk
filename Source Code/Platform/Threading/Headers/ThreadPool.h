/*
   Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _PLATFORM_TASK_POOL_H_
#define _PLATFORM_TASK_POOL_H_

#include "Platform/Headers/PlatformDataTypes.h"

#include <Threadpool-Boost/include/threadpool.hpp>
#include <Threadpool-c++11/Threadpool.hpp>

namespace Divide {

class PoolTask {
public:
    explicit PoolTask(std::function<void()> task);
    explicit PoolTask(I32 priority, std::function<void()> task);

    void operator()();

    I32 _priority;
    std::function<void()> _task;
};

class ThreadPool {
public:
    explicit ThreadPool(I32 numThreads);
    virtual ~ThreadPool();

    virtual bool enqueue(const PoolTask& task) = 0;
    virtual void stopAll() = 0;
    virtual void waitAll() = 0;
    I32 numThreads() const;

protected:
    I32 _numThreads;
};

class ThreadPoolC11 final : public ThreadPool
{
public:
    explicit ThreadPoolC11(I32 numThreads);
    virtual ~ThreadPoolC11();

    bool enqueue(const PoolTask& task) override;
    void stopAll() override;
    void waitAll() override;

private:
    ctpl::thread_pool _pool;
};

class ThreadPoolBoostPrio final : public ThreadPool
{
public:
    explicit ThreadPoolBoostPrio(I32 numThreads);
    virtual ~ThreadPoolBoostPrio();

    bool enqueue(const PoolTask& task) override;
    void stopAll() override;
    void waitAll() override;

private:
    boost::threadpool::prio_pool _pool;
};

class ThreadPoolBoostFifo final : public ThreadPool
{
public:
    explicit ThreadPoolBoostFifo(I32 numThreads);
    virtual ~ThreadPoolBoostFifo();

    bool enqueue(const PoolTask& task) override;
    void stopAll() override;
    void waitAll() override;

private:
    boost::threadpool::fifo_pool _pool;
};

}; //namespace Divide

#endif //_PLATFORM_TASK_POOL_H_