#include "Headers/DirectionalLight.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

DirectionalLight::DirectionalLight()
    : Light(-1, LightType::DIRECTIONAL),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    setRange(to_float(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE));
}

DirectionalLight::~DirectionalLight()
{
}

};