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
    return CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), threadedFunction);
}

TaskHandle CreateTask(PlatformContext& context,
                     TaskHandle* parentTask,
                     const DELEGATE_CBK<void, const Task&>& threadedFunction)
{
    return CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), parentTask, threadedFunction);
}

void WaitForAllTasks(PlatformContext& context, bool yield, bool flushCallbacks, bool foceClear) {
    WaitForAllTasks(context.taskPool(TaskPoolType::HIGH_PRIORITY), yield, flushCallbacks, foceClear);
}

void parallel_for(PlatformContext& context,
                  const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                  U32 count,
                  U32 partitionSize,
                  TaskPriority priority,
                  bool noWait,
                  bool useCurrentThread) {
    parallel_for(context.taskPool(TaskPoolType::HIGH_PRIORITY), cbk, count, partitionSize, priority, noWait, useCurrentThread);
}

}; //namespace Divide