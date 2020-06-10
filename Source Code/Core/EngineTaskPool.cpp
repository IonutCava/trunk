#include "stdafx.h"

#include "Headers/EngineTaskPool.h"

#include "Headers/Kernel.h"
#include "Headers/PlatformContext.h"

namespace Divide {

void WaitForAllTasks(PlatformContext& context, const bool yield, const bool flushCallbacks) {
    WaitForAllTasks(context.taskPool(TaskPoolType::HIGH_PRIORITY), yield, flushCallbacks);
}


void parallel_for(PlatformContext& context, const ParallelForDescriptor& descriptor) {
    parallel_for(context.taskPool(TaskPoolType::HIGH_PRIORITY), descriptor);
}
}; //namespace Divide