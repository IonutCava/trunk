#include "Headers/ParticleSphereVelocityGenerator.h"

namespace Divide {

void ParticleSphereVelocityGenerator::generate(const U64 deltaTime,
                                               ParticleData *p, U32 startIndex,
                                               U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleSphereVelocityGenerator::generate error: Invalid Range!");

    F32 phi, theta, v, r;
    F32 floatPI = to_float(M_PI);
    for (U32 i = startIndex; i < endIndex; ++i) {
        phi = Random(-floatPI, floatPI);
        theta = Random(-floatPI, floatPI);
        v = Random(_minVel, _maxVel);
        r = v * std::sinf(phi);
        p->_velocity[i].z = v * std::cosf(phi);
        p->_velocity[i].x = r * std::cosf(theta);
        p->_velocity[i].y = r * std::sinf(theta);
    }
}
};