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
Task* CreateTask(PlatformContext& context, const DELEGATE_CBK<void, Task&>& threadedFunction, const char* debugName)
{
    return CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), threadedFunction, debugName);
}

Task* CreateTask(PlatformContext& context, Task* parentTask, const DELEGATE_CBK<void, Task&>& threadedFunction, const char* debugName)
{
    return CreateTask(context.taskPool(TaskPoolType::HIGH_PRIORITY), parentTask, threadedFunction, debugName);
}

void WaitForAllTasks(PlatformContext& context, bool yield, bool flushCallbacks, bool foceClear) {
    WaitForAllTasks(context.taskPool(TaskPoolType::HIGH_PRIORITY), yield, flushCallbacks, foceClear);
}

void parallel_for(PlatformContext& context,
                  const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                  const ParallelForDescriptor& descriptor,
                  const char* debugName) {
    parallel_for(context.taskPool(TaskPoolType::HIGH_PRIORITY), cbk, descriptor, debugName);
}

}; //namespace Divide