#include "stdafx.h"

#include "Headers/PointLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

PointLightComponent::PointLightComponent(SceneGraphNode& sgn, F32 range, LightPool& parentPool)
     : Light(sgn, range, LightType::POINT, parentPool),
       SGNComponent<PointLightComponent>(sgn, ComponentType::POINT_LIGHT)
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

    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
}

};