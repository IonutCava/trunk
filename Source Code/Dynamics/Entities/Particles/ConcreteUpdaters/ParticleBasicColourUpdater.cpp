#include "stdafx.h"

#include "Headers/ParticleBasicColourUpdater.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    const U32 g_partitionSize = 128;
};

void ParticleBasicColourUpdater::update(const U64 deltaTimeUS, ParticleData& p) {
   auto parseRange = [&p](Task* parentTask, U32 start, U32 end) -> void {
        for (U32 i = start; i < end; ++i) {
            p._colour[i].set(Lerp(p._startColour[i], p._endColour[i], p._misc[i].y));
        }
    };

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = p.aliveCount();
    descriptor._partitionSize = g_partitionSize;

    parallel_for(_context.context(), parseRange, descriptor);
}
};