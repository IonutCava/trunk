#include "Headers/ParticleBasicColorUpdater.h"

namespace Divide {

void ParticleBasicColorUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    const U32 endID = p->aliveCount();
    for (U32 i = 0; i < endID; ++i) {
        p->_color[i].set(Util::ToByteColor(
            Lerp(p->_startColor[i],
                 p->_endColor[i],
                 p->_misc[i].y)));
    }
}
};