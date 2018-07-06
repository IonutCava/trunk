#include "Headers/ParticleVelocityFromPositionGenerator.h"

namespace Divide {

void ParticleVelocityFromPositionGenerator::generate(vectorImpl<std::future<void>>& packagedTasks, 
                                                     const U64 deltaTime,
                                                     ParticleData& p,
                                                     U32 startIndex,
                                                     U32 endIndex) {
    for (U32 i = startIndex; i < endIndex; ++i) {
        p._velocity[i].xyz(Random(_minScale, _maxScale) *
                           (p._position[i].xyz() - _offset));
    }
}
};