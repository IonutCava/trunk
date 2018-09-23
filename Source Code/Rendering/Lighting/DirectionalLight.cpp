#include "stdafx.h"

#include "Headers/DirectionalLight.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

namespace {
    constexpr F32 g_defaultLightDistance = 500.0f;
};

DirectionalLight::DirectionalLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, LightPool& parentPool)
    : Light(parentCache, descriptorHash, name, -1, LightType::DIRECTIONAL, parentPool),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    // Down the Y and Z axis at DIRECTIONAL_LIGHT_DISTANCE units away;
    _positionAndRange.set(0, -1, -1, g_defaultLightDistance);
    _shadowProperties._lightDetails.y = to_U32(_csmSplitCount);
}

DirectionalLight::~DirectionalLight()
{
}

};