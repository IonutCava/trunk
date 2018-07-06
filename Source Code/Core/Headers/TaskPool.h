/*
Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _TASK_POOL_H_
#define _TASK_POOL_H_

#include "Platform/Threading/Headers/Task.h"
#include <boost/lockfree/queue.hpp>

namespace Divide {

class TaskPool {
  public:
    TaskPool();
    ~TaskPool();

    bool init();
    void flushCallbackQueue();

    TaskHandle getTaskHandle(I64 taskGUID);
    Task& getAvailableTask();
    void setTaskCallback(const TaskHandle& handle,
                         const DELEGATE_CBK<>& callback);

  private:
    //ToDo: replace all friend class declarations with attorneys -Ionut;
    friend class Task;
    void taskCompleted(U32 poolIndex);
    inline ThreadPool& threadPool() {
        return _mainTaskPool;
    }
  private:
    ThreadPool _mainTaskPool;
    boost::lockfree::queue<U32> _threadedCallbackBuffer;

    std::array<Task, Config::MAX_POOLED_TASKS> _tasksPool;
    std::array<bool, Config::MAX_POOLED_TASKS> _taskStates;
    std::array<DELEGATE_CBK<>, Config::MAX_POOLED_TASKS> _taskCallbacks;

    std::atomic<U32> _allocatedJobs;
};

// The following calls should work with any taskpool, 
// but will default to the one created by the kernel

TaskHandle GetTaskHandle(I64 taskGUID);
/**
* @brief Creates a new Task that runs in a separate thread
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction = DELEGATE_CBK<>());

/**
* @brief Creates a new Task that runs in a separate thread
* @param jobIdentifier A unique identifier that gets reset when the job finishes.
*                      Used to check if the local task handle is still valid
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(I64 jobIdentifier,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction = DELEGATE_CBK<>());


//The following calls are identical to the ones above, but use the specified pool
//to schedule tasks
TaskHandle GetTaskHandle(TaskPool& pool,
                         I64 taskGUID);
TaskHandle CreateTask(TaskPool& pool,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction = DELEGATE_CBK<>());
TaskHandle CreateTask(TaskPool& pool, 
                   I64 jobIdentifier,
                   const DELEGATE_CBK_PARAM<bool>& threadedFunction,
                   const DELEGATE_CBK<>& onCompletionFunction = DELEGATE_CBK<>());
/*class CountSplitter
{
public:
explicit CountSplitter(U32 count)
: _count(count)
{
}

template <typename T>
inline bool Split(U32 count) const
{
return (count > _count);
}

private:
U32 _count;
};

class DataSizeSplitter
{
public:
explicit DataSizeSplitter(U32 size)
: _size(size)
{
}

template <typename T>
inline bool Split(U32 count) const
{
return (count*sizeof(T) > _size);
}

private:
U32 _size;
};

template <typename T, typename S>
TaskHandle parallel_for(T* data, U32 count, void(*function)(T*, U32, U32), const S& splitter)
{
typedef parallel_for_job_data<T, S> JobData;
const JobData jobData(data, count, function, splitter);

return CreateTask(-1, &parallel_for_job<JobData>, jobData);
}

template <typename T, typename S>
struct parallel_for_job_data
{
typedef T DataType;
typedef S SplitterType;

parallel_for_job_data(DataType* data, U32 count, void(*function)(DataType*, U32, U32), const SplitterType& splitter)
: data(data)
, count(count)
, function(function)
, splitter(splitter)
{
}

DataType* data;
U32 count;
void(*function)(DataType*, U32, U32);
SplitterType splitter;
};

template <typename JobData>
void parallel_for_job(TaskHandle& job, const void* jobData)
{
const JobData* data = static_cast<const JobData*>(jobData);
const JobData::SplitterType& splitter = data->splitter;

if (splitter.Split<JobData::DataType>(data->count)) {

const U32 leftCount = data->count / 2u;
const JobData leftData(data->data, leftCount, data->function, splitter);
job.addChildTask(CreateTask(job, &parallel_for_job<JobData>, leftData);

const U32 rightCount = data->count - leftCount;
const JobData rightData(data->data + leftCount, rightCount, data->function, splitter);
job.addChildTask(CreateTask(job, &parallel_for_job<JobData>, rightData);
} else {
(data->function)(data->data, data->count);
}
}*/
};

#endif _TASK_POOL_H_