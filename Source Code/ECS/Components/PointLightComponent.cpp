#include "stdafx.h"

#include "Headers/PointLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

PointLightComponent::PointLightComponent(SceneGraphNode& sgn, PlatformContext& context)
     : Light(sgn, 20.0f, LightType::POINT, sgn.parentGraph().parentScene().lightPool()),
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

    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);

    BoundingBox bb;
    bb.setMin(-10.0f);
    bb.setMax(10.0f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);
}

};