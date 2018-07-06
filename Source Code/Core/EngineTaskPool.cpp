#include "Headers/EngineTaskPool.h"
#include "Headers/Kernel.h"

namespace Divide {

TaskHandle GetTaskHandle(I64 taskGUID) {
    return GetTaskHandle(Application::instance().kernel().taskPool(), taskGUID);
}

TaskHandle CreateTask(const DELEGATE_CBK<void, const Task&>& threadedFunction,
    const DELEGATE_CBK<void>& onCompletionFunction)
{
    return CreateTask(-1, threadedFunction, onCompletionFunction);
}

/**
* @brief Creates a new Task that runs in a separate thread
* @param jobIdentifier A unique identifier that gets reset when the job finishes.
*                      Used to check if the local task handle is still valid
* @param threadedFunction The callback function to call in a separate thread = the job to execute
* @param onCompletionFunction The callback function to call when the thread finishes
*/
TaskHandle CreateTask(I64 jobIdentifier,
    const DELEGATE_CBK<void, const Task&>& threadedFunction,
    const DELEGATE_CBK<void>& onCompletionFunction)
{
    TaskPool& pool = Application::instance().kernel().taskPool();
    return CreateTask(pool, jobIdentifier, threadedFunction, onCompletionFunction);
}

void WaitForAllTasks(bool yeld, bool flushCallbacks, bool foceClear) {
    TaskPool& pool = Application::instance().kernel().taskPool();
    WaitForAllTasks(pool, yeld, flushCallbacks, foceClear);
}

TaskHandle parallel_for(const DELEGATE_CBK<void, const Task&, U32, U32>& cbk,
                        U32 count,
                        U32 partitionSize,
                        Task::TaskPriority priority,
                        U32 taskFlags,
                        bool waitForResult)
{
    TaskPool& pool = Application::instance().kernel().taskPool();
    return parallel_for(pool, cbk, count, partitionSize, priority, taskFlags, waitForResult);
}

}; //namespace Divide