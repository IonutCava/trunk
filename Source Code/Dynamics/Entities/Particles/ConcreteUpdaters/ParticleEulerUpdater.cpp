#include "Headers/ParticleEulerUpdater.h"

#include <future>

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec4<F32> globalA(dt * _globalAcceleration, 0.0f);

    const U32 endID = p->aliveCount();

    auto parseRange = [&p, dt, globalA](U32 start, U32 end) -> void {
        vectorImpl<vec4<F32>>& acceleration = p->_acceleration;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& acc = acceleration[i];
            acc.xyz(acc + globalA);
        }

        vectorImpl<vec4<F32>>& velocity = p->_velocity;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& vel = velocity[i];
            vel.xyz(vel + (dt * acceleration[i]));
        }

        vectorImpl<vec4<F32>>& position = p->_position;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& pos = position[i];
            pos.xyz(pos + (dt * velocity[i]));
        }
    };

    const U32 half = endID / 2;

    std::future<void> firstHalf = 
        std::async(std::launch::async | std::launch::deferred,
                   parseRange, 0, half);

    std::future<void> secondHalf =
        std::async(std::launch::async | std::launch::deferred,
            parseRange, half, endID);

    firstHalf.get();
    secondHalf.get();
}
};