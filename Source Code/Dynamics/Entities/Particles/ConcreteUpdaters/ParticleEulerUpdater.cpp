#include "Headers/ParticleEulerUpdater.h"

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, ParticleData *p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec4<F32> globalA(dt * _globalAcceleration.xyz(), 0.0);

    const U32 endID = p->aliveCount();

    for (U32 i = 0; i < endID; ++i) {
        p->_acceleration[i] += globalA;
    }

    for (U32 i = 0; i < endID; ++i) {
        p->_velocity[i] += dt * p->_acceleration[i];
    }

    for (U32 i = 0; i < endID; ++i) {
        p->_position[i] += dt * p->_velocity[i];
    }
}
};