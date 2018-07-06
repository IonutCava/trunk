#include "Headers/ParticleEulerUpdater.h"

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec3<F32> globalA(dt * _globalAcceleration);

    const U32 endID = p->aliveCount();

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& acc = p->_acceleration[i];
        acc.xyz(acc.xyz() + globalA);
    }

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& vel = p->_velocity[i];
        vel.xyz(vel.xyz() + (dt * p->_acceleration[i].xyz()));
    }

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& pos = p->_position[i];
        pos.xyz(pos.xyz() + (dt * p->_velocity[i].xyz()));
    }
}
};