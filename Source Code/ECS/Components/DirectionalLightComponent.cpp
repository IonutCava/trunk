#include "stdafx.h"

#include "Headers/DirectionalLightComponent.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace {
    constexpr F32 g_defaultLightDistance = 500.0f;
};

DirectionalLightComponent::DirectionalLightComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<DirectionalLightComponent, ComponentType::DIRECTIONAL_LIGHT>(sgn, context), 
      Light(sgn, -1, LightType::DIRECTIONAL, sgn.sceneGraph().parentScene().lightPool())
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
    directionField._dataSetter = [this](const void* data) { setDirection(*static_cast<const vec3<F32>*>(data)); };
    directionField._type = EditorComponentFieldType::PUSH_TYPE;
    directionField._readOnly = true;
    directionField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(directionField));



    EditorComponentField sceneFitField = {};
    sceneFitField._name = "Fit CSM To AABB";
    sceneFitField._data = &_csmUseSceneAABBFit;
    sceneFitField._type = EditorComponentFieldType::PUSH_TYPE;
    sceneFitField._readOnly = false;
    sceneFitField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(sceneFitField));

    EditorComponentField csmNearClip = {};
    csmNearClip._name = "CSM Near Clip Offset";
    csmNearClip._data = &_csmNearClipOffset;
    csmNearClip._range = { -g_defaultLightDistance, g_defaultLightDistance };
    csmNearClip._type = EditorComponentFieldType::PUSH_TYPE;
    csmNearClip._readOnly = false;
    csmNearClip._basicType = GFX::PushConstantType::FLOAT;

    getEditorComponent().registerField(std::move(csmNearClip));

    BoundingBox bb;
    bb.setMin(-g_defaultLightDistance * 0.5f);
    bb.setMax(-g_defaultLightDistance * 0.5f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);

    _positionCache.set(VECTOR3_ZERO);
    _feedbackContainers.resize(csmSplitCount());
}

void DirectionalLightComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        Light::updateCache(data);
    }
}

void DirectionalLightComponent::setDirection(const vec3<F32>& direction) {
    TransformComponent* tComp = _parentSGN.get<TransformComponent>();
    if (tComp) {
        Quaternion<F32> rot = tComp->getOrientation();
        rot.fromRotation(directionCache(), direction, tComp->getUpVector());
        tComp->setRotation(rot);
    }
}

};