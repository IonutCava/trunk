#include "Headers/ParticleVelocityGenerator.h"

namespace Divide {

void ParticleVelocityGenerator::generate(const U64 deltaTime, ParticleData *p,
                                         U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleVelocityGenerator::generate error: Invalid Range!");

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_velocity[i].set(random(_minStartVel, _maxStartVel));
    }
}
};