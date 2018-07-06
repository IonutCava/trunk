#include "Headers/SpotLight.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

SpotLight::SpotLight(F32 range) : Light(range, LightType::SPOT)
{
    setRange(2.0f);
    setSpotAngle(35.0f);
    setSpotCosOuterConeAngle(0.65); // 49.5 degrees
    _spotProperties.xyz(WORLD_Z_NEG_AXIS);
}

};