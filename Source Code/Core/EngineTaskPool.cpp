#include "stdafx.h"

#include "Headers/EngineTaskPool.h"

#include "Headers/Kernel.h"
#include "Headers/PlatformContext.h"

namespace Divide {

TaskHandle GetTaskHandle(const PlatformContext& context, I64 taskGUID) {
    return GetTaskHandle(context.app().kernel().taskPool(), taskGUID);
}

TaskHandle CreateTask(const PlatformContext& context, const DELEGATE_CBK<void, const Task&>& threadedFunction,
    const DELEGATE_CBK<void>& onCompletionFunction)
{
    return CreateTask(context, -1, threadedFunction, onCompletionFunction);
}

/**
* @brief Creates a new Task that runs in a separate thread
* @param jobIdentifier A unique identifier that gets reset when the job finishes.
*                      Used to check if the local task handle is still valid
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(const PlatformContext& context, 
    I64 jobIdentifier,
    const DELEGATE_CBK<void, const Task&>& threadedFunction,
    const DELEGATE_CBK<void>& onCompletionFunction)
{
    TaskPool& pool = context.app().kernel().taskPool();
    return CreateTask(pool, jobIdentifier, threadedFunction, onCompletionFunction);
}

void WaitForAllTasks(const PlatformContext& context, bool yield, bool flushCallbacks, bool foceClear) {
    TaskPool& pool = context.app().kernel().taskPool();
    WaitForAllTasks(pool, yield, flushCallbacks, foceClear);
}

TaskHandle parallel_for(const PlatformContext& context,
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority,
                        U32 taskFlags)
{
    TaskPool& pool = context.app().kernel().taskPool();
    return parallel_for(pool, cbk, count, partitionSize, priority, taskFlags);
}

}; //namespace Divide