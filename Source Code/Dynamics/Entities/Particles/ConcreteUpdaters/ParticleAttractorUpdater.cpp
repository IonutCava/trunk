#include "Headers/ParticleAttractorUpdater.h"

namespace Divide {

void ParticleAttractorUpdater::update(const U64 deltaTime, ParticleData& p) {
    const U32 endID = p.aliveCount();
    const vectorAlg::vecSize countAttractors = _attractors.size();

    vec4<F32> offset;
    F32 dist = 0.0f;
    vectorAlg::vecSize a = 0;
    for (U32 i = 0; i < endID; ++i) {
        for (a = 0; a < countAttractors; ++a) {
            offset.set(_attractors[a].xyz() - p._position[i].xyz(), 0.0f);
            dist = offset.dot(offset);

            // if (!IS_ZERO(DIST)) {
            dist = _attractors[a].w / dist;
            p._acceleration[i] += offset * dist;
            //}
        }
    }
}
};