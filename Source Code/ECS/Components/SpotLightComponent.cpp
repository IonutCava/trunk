#include "stdafx.h"

#include "Headers/SpotLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

SpotLightComponent::SpotLightComponent(SceneGraphNode& sgn, PlatformContext& context)
     : Light(sgn, 30.0f, LightType::SPOT, sgn.parentGraph().parentScene().lightPool()),
       BaseComponentType<SpotLightComponent, ComponentType::SPOT_LIGHT>(sgn, context)
{
    setRange(2.0f);
    setConeAngle(35.0f);
    setSpotCosOuterConeAngle(0.65f); // 49.5 degrees

    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
}

};