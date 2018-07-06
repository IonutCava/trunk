#include "Headers/SpotLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

namespace Divide {

SpotLight::SpotLight(F32 range) : Light(range,LIGHT_TYPE_SPOT) 
{
    _properties._position.set(0, 0, 5, 35.0f);
    _properties._direction.set(1.0f, 1.0f, 0.0f, 2.0f);
};

};