#include "stdafx.h"

#include "Headers/ParticleSphereVelocityGenerator.h"

namespace Divide {

void ParticleSphereVelocityGenerator::generate(Task& packagedTasksParent,
                                               const U64 deltaTimeUS,
                                               ParticleData& p,
                                               U32 startIndex,
                                               U32 endIndex) {
    F32 phi, theta, v, r;
    for (U32 i = startIndex; i < endIndex; ++i) {
        phi = Random(-M_PI_f, M_PI_f);
        theta = Random(-M_PI_f, M_PI_f);
        v = Random(_minVel, _maxVel);
        r = v * std::sin(phi);
        vec4<F32>& vel = p._velocity[i];
        vel.set(r * std::cos(theta), r * std::sin(theta), v * std::cos(phi), vel.w);
    }
}
};
