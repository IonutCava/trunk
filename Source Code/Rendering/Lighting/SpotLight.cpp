#include "Headers/SpotLight.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

SpotLight::SpotLight(F32 range) : Light(range, LightType::SPOT) {
    setRange(2.0f);
    setSpotAngle(35.0f);
    setSpotDirection(vec3<F32>(1.0f, 1.0f, 0.0f));
};
};