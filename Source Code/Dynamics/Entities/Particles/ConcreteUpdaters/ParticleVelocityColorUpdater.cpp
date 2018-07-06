#include "Headers/ParticleVelocityColorUpdater.h"

namespace Divide {

void ParticleVelocityColorUpdater::update(const U64 deltaTime,
                                          ParticleData *p) {
    const U32 endID = p->aliveCount();
    F32 diffr = _maxVel.x - _minVel.x;
    F32 diffg = _maxVel.y - _minVel.y;
    F32 diffb = _maxVel.z - _minVel.z;

    vec3<F32> floatColorRGB;
    for (U32 i = 0; i < endID; ++i) {
        floatColorRGB.set(
            (p->_velocity[i].x - _minVel.x) /
                diffr,  // lerp(p->_startColor[i].r, p->_endColor[i].r, scaler),
            (p->_velocity[i].y - _minVel.y) /
                diffg,  // lerp(p->_startColor[i].g, p->_endColor[i].g, scaleg),
            (p->_velocity[i].z - _minVel.z) /
                diffb);  // lerp(p->_startColor[i].b, p->_endColor[i].b,
                         // scaleb),

        p->_color[i].set(
            Util::ToByteColor(floatColorRGB),
            static_cast<U8>(
                Lerp(to_float(p->_startColor[i].a) / 255.0f,
                     to_float(p->_endColor[i].a) / 255.0f,
                     p->_misc[i].y) *
                255.0f));
    }
}
};