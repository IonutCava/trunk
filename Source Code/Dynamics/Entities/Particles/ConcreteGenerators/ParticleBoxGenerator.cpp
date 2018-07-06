#include "Headers/ParticleBoxGenerator.h"

namespace Divide {

void ParticleBoxGenerator::generate(const U64 deltaTime, ParticleData *p,
                                    U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleBoxGenerator::generate error: Invalid Range!");

    vec4<F32> posMin(_pos.xyz() - _maxStartPosOffset.xyz());
    vec4<F32> posMax(_pos.xyz() + _maxStartPosOffset.xyz());

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_position[i].set(Random(posMin, posMax));
    }
}
};