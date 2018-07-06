#include "stdafx.h"

#include "Headers/ParticleFloorUpdater.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

void ParticleFloorUpdater::update(const U64 deltaTimeUS, ParticleData& p) {
    static const U32 s_particlesPerThread = 1024;
    const U32 endID = p.aliveCount();

    STUBBED("ToDo: add proper orientation support! -Ionut")

    F32 floorY = _floorY;
    F32 bounce = _bounceFactor;
    auto updateFloor = [&p, floorY, bounce](const Task& parentTask, U32 start, U32 end) {
        for (U32 i = start; i < end; ++i) {
            if ((p._position[i].y - (p._position[i].w / 2)) < floorY) {
                vec3<F32> force(p._acceleration[i]);

                F32 normalFactor = force.dot(WORLD_Y_AXIS);
                if (normalFactor < 0.0f) {
                    force -= WORLD_Y_AXIS * normalFactor;
                }
                F32 velFactor = p._velocity[i].xyz().dot(WORLD_Y_AXIS);
                // if (velFactor < 0.0)
                p._velocity[i] -= vec4<F32>(WORLD_Y_AXIS * (1.0f + bounce) * velFactor, 0.0f);
                p._acceleration[i].xyz(force);
            }
        }
    };

    parallel_for(_context.parent().platformContext(), updateFloor, endID, s_particlesPerThread);
}

};