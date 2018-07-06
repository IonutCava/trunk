#include "Headers/ParticleVelocityFromPositionGenerator.h"

namespace Divide {

void ParticleVelocityFromPositionGenerator::generate(const U64 deltaTime,
                                                     ParticleData *p,
                                                     U32 startIndex,
                                                     U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(p->_position.size(), startIndex, endIndex),
        "ParticleVelocityFromPositionGenerator::generate error: Invalid "
        "Range!");

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_velocity[i].set(random(_minScale, _maxScale) *
                            (p->_position[i] - _offset));
    }
}
};