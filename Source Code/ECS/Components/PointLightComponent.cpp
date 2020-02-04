#include "stdafx.h"

#include "Headers/PointLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

PointLightComponent::PointLightComponent(SceneGraphNode& sgn, PlatformContext& context)
     : Light(sgn, 20.0f, LightType::POINT, sgn.sceneGraph().parentScene().lightPool()),
       BaseComponentType<PointLightComponent, ComponentType::POINT_LIGHT>(sgn, context)
{
    // +x
    _direction[0].set(WORLD_X_AXIS);
    // -x
    _direction[1].set(WORLD_X_NEG_AXIS);
    // +z
    _direction[2].set(WORLD_Z_AXIS);
    // -z
    _direction[3].set(WORLD_Z_NEG_AXIS);
    // +y
    _direction[4].set(WORLD_Y_AXIS);
    // -y
    _direction[5].set(WORLD_Y_NEG_AXIS);

    _shadowProperties._lightDetails.z = 0.05f;


    EditorComponentField rangeAndConeField = {};
    rangeAndConeField._name = "Range and Cone";
    rangeAndConeField._data = &_rangeAndCones;
    rangeAndConeField._type = EditorComponentFieldType::PUSH_TYPE;
    rangeAndConeField._readOnly = false;
    rangeAndConeField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(rangeAndConeField));

    BoundingBox bb;
    bb.setMin(-10.0f);
    bb.setMax(10.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);

    _directionCache.set(VECTOR3_ZERO);
}

void PointLightComponent::OnData(const ECS::Data& data) {
    if (data.eventType == ECSCustomEventType::TransformUpdated) {
        Light::updateCache();
    }
}

};