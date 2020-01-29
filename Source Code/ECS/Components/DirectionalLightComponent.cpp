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
      Light(sgn, -1, LightType::DIRECTIONAL, sgn.sceneGraph().parentScene().lightPool()),
      _csmSplitCount(3),
      _csmNearClipOffset(100.0f)
{
    setRange(g_defaultLightDistance);
    _shadowProperties._lightDetails.y = to_F32(_csmSplitCount);
    _shadowProperties._lightDetails.z = 0.0f;
    csmSplitCount(context.config().rendering.shadowMapping.defaultCSMSplitCount);

    EditorComponentField rangeAndConeField = {};
    rangeAndConeField._name = "Range and Cone";
    rangeAndConeField._data = &_rangeAndCones;
    rangeAndConeField._type = EditorComponentFieldType::PUSH_TYPE;
    rangeAndConeField._readOnly = false;
    rangeAndConeField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(rangeAndConeField));

    EditorComponentField directionField = {};
    directionField._name = "Direction";
    directionField._dataGetter = [this](void* dataOut) { static_cast<vec3<F32>*>(dataOut)->set(directionCache()); };
    directionField._dataSetter = [this](const void* data) { /*NOP*/ ACKNOWLEDGE_UNUSED(data); }; 
    directionField._type = EditorComponentFieldType::PUSH_TYPE;
    directionField._readOnly = true;
    rangeAndConeField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(directionField));

    BoundingBox bb;
    bb.setMin(-g_defaultLightDistance * 0.5f);
    bb.setMax(-g_defaultLightDistance * 0.5f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);
}

DirectionalLightComponent::~DirectionalLightComponent()
{
}

void DirectionalLightComponent::OnData(const ECS::Data& data) {
    if (data.eventType == ECSCustomEventType::TransformUpdated) {
        Light::updateCache();
    }
}
};