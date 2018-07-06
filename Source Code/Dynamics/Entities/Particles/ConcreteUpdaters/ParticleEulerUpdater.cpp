#include "Headers/ParticleEulerUpdater.h"

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec4<F32> globalA(dt * _globalAcceleration, 0.0f);

    const U32 endID = p->aliveCount();

    vectorImpl<vec4<F32>>& acceleration = p->_acceleration;
    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& acc = acceleration[i];
        acc.xyz(acc + globalA);
    }

    vectorImpl<vec4<F32>>& velocity = p->_velocity;
    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& vel = velocity[i];
        vel.xyz(vel + (dt * acceleration[i]));
    }

    vectorImpl<vec4<F32>>& position = p->_position;
    for (U32 i = 0; i < endID; ++i) {
        vec4<F32>& pos = position[i];
        pos.xyz(pos + (dt * velocity[i]));
    }
}
};