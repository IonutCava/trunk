#include "Headers/ParticleBasicColorUpdater.h"

namespace Divide {

void ParticleBasicColorUpdater::update(const U64 deltaTime, ParticleData *p) {
    const U32 endId = p->aliveCount();
    for (U32 i = 0; i < endId; ++i) {
        p->_color[i].set(Util::toByteColor(
            lerp(Util::toFloatColor(p->_startColor[i]),
                 Util::toFloatColor(p->_endColor[i]), p->_misc[i].y)));
    }
}
};