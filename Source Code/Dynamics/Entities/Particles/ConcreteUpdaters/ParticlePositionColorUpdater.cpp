#include "Headers/ParticlePositionColorUpdater.h"

namespace Divide {

void ParticlePositionColorUpdater::update(const U64 deltaTime,
                                          ParticleData *p) {
    const U32 endID = p->aliveCount();
    F32 diffr = _maxPos.x - _minPos.x;
    F32 diffg = _maxPos.y - _minPos.y;
    F32 diffb = _maxPos.z - _minPos.z;

    vec3<F32> floatColorRGB;
    for (U32 i = 0; i < endID; ++i) {
        floatColorRGB.set(
            (p->_position[i].x - _minPos.x) /
                diffr,  // lerp(p->_startColor[i].r, p->_endColor[i].r, scaler),
            (p->_position[i].y - _minPos.y) /
                diffg,  // lerp(p->_startColor[i].g, p->_endColor[i].g, scaleg),
            (p->_position[i].z - _minPos.z) /
                diffb);  // lerp(p->_startColor[i].b, p->_endColor[i].b,
                         // scaleb),
        p->_color[i].set(
            Util::ToByteColor(floatColorRGB),
            to_ubyte(
                Lerp(to_float(p->_startColor[i].a) / 255.0f,
                     to_float(p->_endColor[i].a) / 255.0f,
                     p->_misc[i].y) *
                255.0f));
    }
}
};