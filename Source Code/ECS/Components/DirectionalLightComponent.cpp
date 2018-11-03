#include "stdafx.h"

#include "Headers/DirectionalLightComponent.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

namespace {
    constexpr F32 g_defaultLightDistance = 500.0f;
};

DirectionalLightComponent::DirectionalLightComponent(SceneGraphNode& sgn, LightPool& parentPool)
    : Light(sgn, -1, LightType::DIRECTIONAL, parentPool),
      SGNComponent<DirectionalLightComponent>(sgn, ComponentType::DIRECTIONAL_LIGHT),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    setRange(g_defaultLightDistance);
    _shadowProperties._lightDetails.y = to_U32(_csmSplitCount);
    csmSplitCount(parentPool.context().config().rendering.shadowMapping.defaultCSMSplitCount);
    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
}

DirectionalLightComponent::~DirectionalLightComponent()
{
}

};