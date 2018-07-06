#include "Headers/ParticleSphereVelocityGenerator.h"

namespace Divide {

void ParticleSphereVelocityGenerator::generate(TaskHandle& packagedTasksParent,
                                               const U64 deltaTime,
                                               ParticleData& p,
                                               U32 startIndex,
                                               U32 endIndex) {

    F32 phi, theta, v, r;
    F32 floatPI = to_F32(M_PI);
    for (U32 i = startIndex; i < endIndex; ++i) {
        phi = Random(-floatPI, floatPI);
        theta = Random(-floatPI, floatPI);
        v = Random(_minVel, _maxVel);
        r = v * std::sin(phi);
        vec4<F32>& vel = p._velocity[i];
        vel.set(r * std::cos(theta), r * std::sin(theta), v * std::cos(phi), vel.w);
    }
}
};
