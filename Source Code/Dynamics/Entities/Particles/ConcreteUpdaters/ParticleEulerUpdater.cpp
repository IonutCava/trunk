#include "Headers/ParticleEulerUpdater.h"

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, ParticleData *p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec3<F32> globalA(dt * _globalAcceleration);

    const U32 endID = p->aliveCount();

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& temp = p->_acceleration[i];
        temp.xyz(temp.xyz() + globalA);
    }

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& temp = p->_velocity[i];
        temp.xyz(temp.xyz() + (dt * p->_acceleration[i].xyz()));
    }

    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& temp = p->_position[i];
        temp.xyz(temp.xyz() + (dt * p->_velocity[i].xyz()));
    }
}
};