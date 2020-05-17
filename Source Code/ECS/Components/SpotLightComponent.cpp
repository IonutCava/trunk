#include "stdafx.h"

#include "Headers/SpotLightComponent.h"

#include "Core/Headers/PlatformContext.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

SpotLightComponent::SpotLightComponent(SceneGraphNode& sgn, PlatformContext& context)
     : Light(sgn, 30.0f, LightType::SPOT, sgn.sceneGraph().parentScene().lightPool()),
       BaseComponentType<SpotLightComponent, ComponentType::SPOT_LIGHT>(sgn, context)
{

    EditorComponentField cutoffAngle = {};
    cutoffAngle._name = "Cutoff Angle";
    cutoffAngle._data = &_coneCutoffAngle;
    cutoffAngle._type = EditorComponentFieldType::PUSH_TYPE;
    cutoffAngle._readOnly = false;
    cutoffAngle._range = { std::numeric_limits<F32>::epsilon(), 179.99f };
    cutoffAngle._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(std::move(cutoffAngle));

    EditorComponentField outerCutoffAngle = {};
    outerCutoffAngle._name = "Outer Cutoff Angle";
    outerCutoffAngle._data = &_outerConeCutoffAngle;
    outerCutoffAngle._type = EditorComponentFieldType::PUSH_TYPE;
    outerCutoffAngle._readOnly = false;
    outerCutoffAngle._range = { std::numeric_limits<F32>::epsilon(), 180.0f };
    outerCutoffAngle._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(std::move(outerCutoffAngle));

    EditorComponentField directionField = {};
    directionField._name = "Direction";
    directionField._dataGetter = [this](void* dataOut) { static_cast<vec3<F32>*>(dataOut)->set(directionCache()); };
    directionField._dataSetter = [this](const void* data) { setDirection(*static_cast<const vec3<F32>*>(data)); };
    directionField._type = EditorComponentFieldType::PUSH_TYPE;
    directionField._readOnly = true;
    directionField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(directionField));

    EditorComponentField showConeField = {};
    showConeField._name = "Show direction cone";
    showConeField._data = &_showDirectionCone;
    showConeField._type = EditorComponentFieldType::PUSH_TYPE;
    showConeField._readOnly = false;
    showConeField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(showConeField));

    registerFields(getEditorComponent());

    getEditorComponent().onChangedCbk([this](std::string_view field) {
        if (coneCutoffAngle() > outerConeCutoffAngle()) {
            coneCutoffAngle(outerConeCutoffAngle());
        }
    });

    BoundingBox bb = {};
    bb.setMin(-1.0f);
    bb.setMax(1.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);
}

F32 SpotLightComponent::outerConeRadius() const noexcept {
    return range() * std::tan(Angle::DegreesToRadians(outerConeCutoffAngle()));
}

F32 SpotLightComponent::innerConeRadius() const noexcept {
    return range() * std::tan(Angle::DegreesToRadians(coneCutoffAngle()));
}

F32 SpotLightComponent::coneSlantHeight() const noexcept {
    const F32 coneRadius = outerConeRadius();
    const F32 coneHeight = range();

    return Sqrt(SQUARED(coneRadius) + SQUARED(coneHeight));
}

void SpotLightComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<SpotLightComponent, ComponentType::SPOT_LIGHT>;

    if (_drawImpostor) {
        _parentPool.context().gfx().debugDrawCone(positionCache(), directionCache(), range(), outerConeRadius(), getDiffuseColour());
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

void SpotLightComponent::setDirection(const vec3<F32>& direction) {
    TransformComponent* tComp = _parentSGN.get<TransformComponent>();
    if (tComp) {
        Quaternion<F32> rot = tComp->getOrientation();
        rot.fromRotation(directionCache(), direction, tComp->getUpVector());
        tComp->setRotation(rot);
    }
}
};