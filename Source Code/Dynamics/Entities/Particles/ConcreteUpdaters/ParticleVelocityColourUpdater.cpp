#include "stdafx.h"

#include "Headers/ParticleVelocityColourUpdater.h"

namespace Divide {

void ParticleVelocityColourUpdater::update(const U64 deltaTime, ParticleData& p) {
    const U32 endID = p.aliveCount();
    F32 diffr = _maxVel.x - _minVel.x;
    F32 diffg = _maxVel.y - _minVel.y;
    F32 diffb = _maxVel.z - _minVel.z;

    vec3<F32> floatColourRGB;
    for (U32 i = 0; i < endID; ++i) {
        p._colour[i].set(
            (p._velocity[i].x - _minVel.x) /
                diffr,  // lerp(p._startColour[i].r, p._endColour[i].r, scaler),
            (p._velocity[i].y - _minVel.y) /
                diffg,  // lerp(p._startColour[i].g, p._endColour[i].g, scaleg),
            (p._velocity[i].z - _minVel.z) /
                diffb,  // lerp(p._startColour[i].b, p._endColour[i].b,
                         // scaleb),
            Lerp(p._startColour[i].a, p._endColour[i].a, p._misc[i].y) * 255.0f);
    }
}
};