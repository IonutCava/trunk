#include "Headers/ParticleBasicColourUpdater.h"
#include "Core/Headers/TaskPool.h"

namespace Divide {

namespace {
    const U32 g_partitionSize = 128;
};

void ParticleBasicColourUpdater::update(const U64 deltaTime, ParticleData& p) {
   auto parseRange = [&p](const std::atomic_bool& stopRequested, U32 start, U32 end) -> void {
        for (U32 i = start; i < end; ++i) {
            p._colour[i].set(Lerp(p._startColour[i], p._endColour[i], p._misc[i].y));
        }
    };

    parallel_for(parseRange, p.aliveCount(), g_partitionSize);
}
};