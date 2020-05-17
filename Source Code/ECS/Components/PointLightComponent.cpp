#include "stdafx.h"

#include "Headers/PointLightComponent.h"

#include "Core/Headers/PlatformContext.h"
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

    registerFields(getEditorComponent());

    BoundingBox bb = {};
    bb.setMin(-10.0f);
    bb.setMax(10.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);

    _directionCache.set(VECTOR3_ZERO);
}

void PointLightComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<PointLightComponent, ComponentType::POINT_LIGHT>;

    if (_drawImpostor) {
        _parentPool.context().gfx().debugDrawSphere(positionCache(), 0.5f, getDiffuseColour());
        _parentPool.context().gfx().debugDrawSphere(positionCache(), range(), DefaultColours::GREEN);
    }

    Parent::PreUpdate(deltaTime);
}

void PointLightComponent::OnData(const ECS::CustomEvent& data) {
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