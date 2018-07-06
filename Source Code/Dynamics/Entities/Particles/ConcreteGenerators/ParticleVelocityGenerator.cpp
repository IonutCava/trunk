#include "Headers/ParticleVelocityGenerator.h"

namespace Divide {

void ParticleVelocityGenerator::generate(const U64 deltaTime,
                                         std::shared_ptr<ParticleData> p,
                                         U32 startIndex,
                                         U32 endIndex) {
    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_velocity[i].set(Random(_minStartVel, _maxStartVel));
    }
}
};