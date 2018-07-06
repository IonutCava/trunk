#include "Headers/ParticleTimeGenerator.h"

namespace Divide {

void ParticleTimeGenerator::generate(vectorImpl<std::future<void>>& packagedTasks, 
                                     const U64 deltaTime,
                                     ParticleData& p,
                                     U32 startIndex,
                                     U32 endIndex) {
    for (U32 i = startIndex; i < endIndex; ++i) {
        F32 time = Random(_minTime, _maxTime);
        p._misc[i].x = time;
        p._misc[i].y = 0.0f;
        p._misc[i].z = time > EPSILON_F32 ? 1.0f / p._misc[i].x : 0.0f;
    }
}
};