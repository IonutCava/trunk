#include "stdafx.h"

#include "Headers/SpotLightComponent.h"

#include "Core/Headers/PlatformContext.h"
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

    EditorComponentField colourField = {};
    colourField._name = "Colour";
    colourField._dataGetter = [this](void* dataOut) { static_cast<FColour3*>(dataOut)->set(getDiffuseColour()); };
    colourField._dataSetter = [this](const void* data) { setDiffuseColour(*static_cast<const FColour3*>(data)); };
    colourField._type = EditorComponentFieldType::PUSH_TYPE;
    colourField._readOnly = false;
    colourField._basicType = GFX::PushConstantType::FCOLOUR3;

    EditorComponentField rangeAndConeField = {};
    rangeAndConeField._name = "Range and Cone";
    rangeAndConeField._data = &_rangeAndCones;
    rangeAndConeField._type = EditorComponentFieldType::PUSH_TYPE;
    rangeAndConeField._readOnly = false;
    rangeAndConeField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(rangeAndConeField));

    BoundingBox bb = {};
    bb.setMin(-1.0f);
    bb.setMax(1.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);
}

void SpotLightComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<SpotLightComponent, ComponentType::SPOT_LIGHT>;

    if (_drawImpostor) {
        const F32 coneRadius = getRange() * std::tan(getConeAngle());
        _parentPool.context().gfx().debugDrawCone(positionCache(), directionCache(), getRange(), coneRadius, getDiffuseColour());
    }

    Parent::PreUpdate(deltaTime);
}

void SpotLightComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        Light::updateCache(data);
    } else if (data._type == ECS::CustomEvent::Type::EntityFlagChanged) {
        const SceneGraphNode::Flags flag = static_cast<SceneGraphNode::Flags>(std::any_cast<U32>(data._flag));
        if (flag == SceneGraphNode::Flags::SELECTED) {
            const auto [state, recursive] = std::any_cast<std::pair<bool, bool>>(data._userData);
            _drawImpostor = state;
        }
    }
}
};