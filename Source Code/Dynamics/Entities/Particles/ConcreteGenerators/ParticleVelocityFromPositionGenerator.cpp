#include "Headers/ParticleVelocityFromPositionGenerator.h"

namespace Divide {

void ParticleVelocityFromPositionGenerator::generate(const U64 deltaTime,
                                                     ParticleData *p,
                                                     U32 startIndex,
                                                     U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleVelocityFromPositionGenerator::generate error: Invalid "
        "Range!");

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_velocity[i].xyz(Random(_minScale, _maxScale) *
                            (p->_position[i].xyz() - _offset));
    }
}
};