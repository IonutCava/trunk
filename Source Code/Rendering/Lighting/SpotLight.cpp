#include "Headers/SpotLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

SpotLight::SpotLight(U8 slot, F32 range) : Light(slot,range,LIGHT_TYPE_SPOT) {
    _physicalProperties._spotCutoff = 35;
    _physicalProperties._position = vec4<F32>(0, 0, 5, 2.0f);
    _physicalProperties._direction = vec4<F32>(1.0f, 1.0f, 0.0f, 2.0f);
};