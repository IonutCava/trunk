#include "Headers/ParticleFloorUpdater.h"

namespace Divide {

void ParticleFloorUpdater::update(const U64 deltaTime, ParticleData *p) {
    const U32 endId = p->aliveCount();

    vec3<F32> force;
    for (U32 i = 0; i < endId; ++i) {
        if (p->_position[i].y < _floorY) {
            force.set(p->_acceleration[i]);

            F32 normalFactor = force.dot(WORLD_Y_AXIS);
            if (normalFactor < 0.0f) {
                force -= WORLD_Y_AXIS * normalFactor;
            }
            F32 velFactor = p->_velocity[i].xyz().dot(WORLD_Y_AXIS);
            // if (velFactor < 0.0)
            p->_velocity[i] -= vec4<F32>(WORLD_Y_AXIS, 0.0f) *
                               (1.0f + _bounceFactor) * velFactor;
            p->_acceleration[i] = force;
        }
    }
}
};