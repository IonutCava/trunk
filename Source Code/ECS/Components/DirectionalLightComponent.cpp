#include "stdafx.h"

#include "Headers/DirectionalLightComponent.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

namespace {
    constexpr F32 g_defaultLightDistance = 500.0f;
};

DirectionalLightComponent::DirectionalLightComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<DirectionalLightComponent, ComponentType::DIRECTIONAL_LIGHT>(sgn, context), 
      Light(sgn, -1, LightType::DIRECTIONAL, sgn.parentGraph().parentScene().lightPool()),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    setRange(g_defaultLightDistance);
    _shadowProperties._lightDetails.y = to_U32(_csmSplitCount);
    csmSplitCount(context.config().rendering.shadowMapping.defaultCSMSplitCount);
    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
}

DirectionalLightComponent::~DirectionalLightComponent()
{
}

};