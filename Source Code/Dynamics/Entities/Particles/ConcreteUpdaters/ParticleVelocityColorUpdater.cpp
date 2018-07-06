#include "Headers/ParticleVelocityColorUpdater.h"

namespace Divide {

void ParticleVelocityColorUpdater::update(const U64 deltaTime, ParticleData& p) {
    const U32 endID = p.aliveCount();
    F32 diffr = _maxVel.x - _minVel.x;
    F32 diffg = _maxVel.y - _minVel.y;
    F32 diffb = _maxVel.z - _minVel.z;

    vec3<F32> floatColorRGB;
    for (U32 i = 0; i < endID; ++i) {
        p._color[i].set(
            (p._velocity[i].x - _minVel.x) /
                diffr,  // lerp(p._startColor[i].r, p._endColor[i].r, scaler),
            (p._velocity[i].y - _minVel.y) /
                diffg,  // lerp(p._startColor[i].g, p._endColor[i].g, scaleg),
            (p._velocity[i].z - _minVel.z) /
                diffb,  // lerp(p._startColor[i].b, p._endColor[i].b,
                         // scaleb),
            Lerp(p._startColor[i].a, p._endColor[i].a, p._misc[i].y) * 255.0f);
    }
}
};