#include "Headers/ParticleColorGenerator.h"

namespace Divide {

void ParticleColorGenerator::generate(const U64 deltaTime,
                                      std::shared_ptr<ParticleData> p,
                                      U32 startIndex,
                                      U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleColorGenerator::generate error: Invalid Range!");

    const vec4<F32> minStartCol(Util::ToFloatColor(_minStartCol));
    const vec4<F32> maxStartCol(Util::ToFloatColor(_maxStartCol));
    const vec4<F32> minEndCol(Util::ToFloatColor(_minEndCol));
    const vec4<F32> maxEndCol(Util::ToFloatColor(_maxEndCol));

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_startColor[i].set(Random(minStartCol, maxStartCol));
        p->_endColor[i].set(Random(minEndCol, maxEndCol));
    }
}
};