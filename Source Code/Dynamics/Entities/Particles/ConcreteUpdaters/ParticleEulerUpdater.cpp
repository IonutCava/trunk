#include "stdafx.h"

#include "Headers/ParticleEulerUpdater.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    const U32 g_partitionSize = 256;
};

void ParticleEulerUpdater::update(const U64 deltaTimeUS, ParticleData& p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
    const vec4<F32> globalA(dt * _globalAcceleration, 0.0f);

    const U32 endID = p.aliveCount();

    auto parseRange = [&p, dt, globalA](const Task* parentTask, U32 start, U32 end) -> void {
        vectorBest<vec4<F32>>& acceleration = p._acceleration;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& acc = acceleration[i];
            acc.xyz(acc + globalA);
        }
        vectorBest<vec4<F32>>& velocity = p._velocity;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& vel = velocity[i];
            vel.xyz(vel + (dt * acceleration[i]));
        }

        vectorBest<vec4<F32>>& position = p._position;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& pos = position[i];
            pos.xyz(pos + (dt * velocity[i]));
        }
    };

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = endID;
    descriptor._partitionSize = g_partitionSize;

    parallel_for(_context.context(), parseRange, descriptor);
}

};