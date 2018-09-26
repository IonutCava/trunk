#include "stdafx.h"

#include "Headers/SpotLight.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

SpotLight::SpotLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, F32 range, LightPool& parentPool)
     : Light(parentCache, descriptorHash, name, range, LightType::SPOT, parentPool)
{
    setRange(2.0f);
    setConeAngle(35.0f);
    setSpotCosOuterConeAngle(0.65f); // 49.5 degrees
    _directionAndCone.xyz(WORLD_Z_NEG_AXIS);
}

};