#include "stdafx.h"

#include "Headers/SpotLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

SpotLightComponent::SpotLightComponent(SceneGraphNode& sgn, PlatformContext& context)
     : Light(sgn, 30.0f, LightType::SPOT, sgn.sceneGraph().parentScene().lightPool()),
       BaseComponentType<SpotLightComponent, ComponentType::SPOT_LIGHT>(sgn, context)
{
    setRange(2.0f);
    setConeAngle(35.0f);
    setSpotCosOuterConeAngle(std::cos(Angle::to_RADIANS(49.5f)));
    _shadowProperties._lightDetails.z = 0.05f;

    EditorComponentField rangeAndConeField = {};
    rangeAndConeField._name = "Range and Cone";
    rangeAndConeField._data = &_rangeAndCones;
    rangeAndConeField._type = EditorComponentFieldType::PUSH_TYPE;
    rangeAndConeField._readOnly = false;
    rangeAndConeField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(rangeAndConeField));

    BoundingBox bb;
    bb.setMin(-1.0f);
    bb.setMax(1.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);
}

void SpotLightComponent::OnData(const ECS::Data& data) {
    if (data.eventType == ECSCustomEventType::TransformUpdated) {
        Light::updateCache();
    }
}
};