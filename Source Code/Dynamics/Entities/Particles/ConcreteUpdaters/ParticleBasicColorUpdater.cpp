#include "Headers/ParticleBasicColorUpdater.h"

namespace Divide {

void ParticleBasicColorUpdater::update(const U64 deltaTime, ParticleData *p) {
    const U32 endID = p->aliveCount();
    for (U32 i = 0; i < endID; ++i) {
        p->_color[i].set(Util::ToByteColor(
            Lerp(Util::ToFloatColor(p->_startColor[i]),
                 Util::ToFloatColor(p->_endColor[i]), p->_misc[i].y)));
    }
}
};