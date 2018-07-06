#include "Headers/ParticleBasicTimeUpdater.h"

namespace Divide {

void ParticleBasicTimeUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    U32 endID = p->aliveCount();
    const F32 localDT = Time::MicrosecondsToSeconds<F32>(deltaTime);

    if (endID == 0) {
        return;
    }

    for (U32 i = 0; i < endID; ++i) {
        p->_misc[i].x -= localDT;
        // interpolation: from 0 (start of life) till 1 (end of life)
        p->_misc[i].y =
            1.0f - (p->_misc[i].x * p->_misc[i].z);  // .z is 1.0/max life time

        if (p->_misc[i].x <= 0.0f) {
            p->kill(i);
            endID = p->aliveCount() < p->totalCount() ? p->aliveCount()
                                                      : p->totalCount();
        }
    }
}
};