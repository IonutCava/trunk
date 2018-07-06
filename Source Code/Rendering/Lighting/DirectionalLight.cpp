#include "stdafx.h"

#include "Headers/DirectionalLight.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

DirectionalLight::DirectionalLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, LightPool& parentPool)
    : Light(parentCache, descriptorHash, name, -1, LightType::DIRECTIONAL, parentPool),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    // Down the Y and Z axis at DIRECTIONAL_LIGHT_DISTANCE units away;
    _positionAndRange.set(0, -1, -1, to_F32(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE));
}

DirectionalLight::~DirectionalLight()
{
}

};