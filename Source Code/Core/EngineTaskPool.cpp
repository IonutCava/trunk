#include "stdafx.h"

#include "Headers/EngineTaskPool.h"

#include "Headers/Kernel.h"
#include "Headers/PlatformContext.h"

namespace Divide {

/**
* @brief Creates a new Task that runs in a separate thread
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(PlatformContext& context, 
                      const DELEGATE_CBK<void, const Task&>& threadedFunction)
{
    return CreateTask(context.taskPool(), threadedFunction);
}

TaskHandle CreateTask(PlatformContext& context,
                     TaskHandle* parentTask,
                     const DELEGATE_CBK<void, const Task&>& threadedFunction)
{
    return CreateTask(context.taskPool(), parentTask, threadedFunction);
}

void WaitForAllTasks(PlatformContext& context, bool yield, bool flushCallbacks, bool foceClear) {
    TaskPool& pool = context.taskPool();
    WaitForAllTasks(pool, yield, flushCallbacks, foceClear);
}

TaskHandle parallel_for(PlatformContext& context,
                        const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        TaskPriority priority)
{
    TaskPool& pool = context.taskPool();
    return parallel_for(pool, cbk, count, partitionSize, priority);
}

}; //namespace Divide