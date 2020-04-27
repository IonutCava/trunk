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
#ifndef _TASK_POOL_INL_
#define _TASK_POOL_INL_

namespace Divide {
    template<class Predicate>
    Task* TaskPool::createTask(Task* parentTask, Predicate&& threadedFunction, bool allowedInIdle) {
        Task* task = allocateTask(parentTask, allowedInIdle);
        task->_callback = std::move(threadedFunction);
        return task;
    }

    template<class Predicate>
    Task* CreateTask(TaskPool& pool, Predicate&& threadedFunction, bool allowedInIdle) {
        return CreateTask(pool, nullptr, std::move(threadedFunction), allowedInIdle);
    }

    template<class Predicate>
    Task* CreateTask(TaskPool& pool, Task* parentTask, Predicate&& threadedFunction, bool allowedInIdle) {
        return pool.createTask(parentTask, std::move(threadedFunction), allowedInIdle);
    }

    template<class Predicate>
    void parallel_for(TaskPool& pool, Predicate&& cbk, const ParallelForDescriptor& descriptor) {
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
                                                [cbk, &jobCount, start, end](Task& parentTask) {
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
                                                [cbk, &jobCount, count, remainder](Task& parentTask) {
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
}; //namespace Divide

#endif //_TASK_POOL_INL_
