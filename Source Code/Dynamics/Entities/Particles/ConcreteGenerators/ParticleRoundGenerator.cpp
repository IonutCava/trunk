#include "Headers/ParticleRoundGenerator.h"

namespace Divide {

void ParticleRoundGenerator::generate(const U64 deltaTime, ParticleData *p,
                                      U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleBoxGenerator::generate error: Invalid Range!");

    for (U32 i = startIndex; i < endIndex; i++) {
        F32 ang = Random(0.0f, to_float(M_2PI));
        p->_position[i].xyz(_center + vec3<F32>(_radX * std::sin(ang),
                                                _radY * std::cos(ang), 
                                                0.0f));
    }
}
};