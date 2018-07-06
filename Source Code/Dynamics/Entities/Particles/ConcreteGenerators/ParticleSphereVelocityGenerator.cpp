#include "Headers/ParticleSphereVelocityGenerator.h"

namespace Divide {

void ParticleSphereVelocityGenerator::generate(TaskHandle& packagedTasksParent,
                                               const U64 deltaTime,
                                               ParticleData& p,
                                               U32 startIndex,
                                               U32 endIndex) {

    F32 phi, theta, v, r;
    F32 floatPI = to_float(M_PI);
    for (U32 i = startIndex; i < endIndex; ++i) {
        phi = Random(-floatPI, floatPI);
        theta = Random(-floatPI, floatPI);
        v = Random(_minVel, _maxVel);
        r = v * std::sin(phi);
        p._velocity[i].z = v * std::cos(phi);
        p._velocity[i].x = r * std::cos(theta);
        p._velocity[i].y = r * std::sin(theta);
    }
}
};
