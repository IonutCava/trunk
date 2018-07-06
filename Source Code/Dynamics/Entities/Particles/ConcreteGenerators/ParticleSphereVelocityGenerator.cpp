#include "Headers/ParticleSphereVelocityGenerator.h"

namespace Divide {

void ParticleSphereVelocityGenerator::generate(const U64 deltaTime,
                                               ParticleData *p, U32 startIndex,
                                               U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(static_cast<U32>(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleSphereVelocityGenerator::generate error: Invalid Range!");

    F32 phi, theta, v, r;
    for (U32 i = startIndex; i < endIndex; ++i) {
        phi = random(-M_PI, M_PI);
        theta = random(-M_PI, M_PI);
        v = random(_minVel, _maxVel);
        r = v * std::sinf(phi);
        p->_velocity[i].z = v * std::cosf(phi);
        p->_velocity[i].x = r * std::cosf(theta);
        p->_velocity[i].y = r * std::sinf(theta);
    }
}
};