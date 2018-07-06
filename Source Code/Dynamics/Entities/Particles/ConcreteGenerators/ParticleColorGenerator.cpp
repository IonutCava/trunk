#include "Headers/ParticleColorGenerator.h"

namespace Divide {

void ParticleColorGenerator::generate(const U64 deltaTime, ParticleData *p,
                                      U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(static_cast<U32>(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleColorGenerator::generate error: Invalid Range!");

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_startColor[i].set(random(_minStartCol, _maxStartCol));
        p->_endColor[i].set(random(_minEndCol, _maxEndCol));
    }
}
};