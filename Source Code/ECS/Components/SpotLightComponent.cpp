#include "stdafx.h"

#include "Headers/SpotLightComponent.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

SpotLightComponent::SpotLightComponent(SceneGraphNode& sgn, F32 range, LightPool& parentPool)
     : Light(sgn, range, LightType::SPOT, parentPool),
       SGNComponent<SpotLightComponent>(sgn, ComponentType::SPOT_LIGHT)
{
    setRange(2.0f);
    setConeAngle(35.0f);
    setSpotCosOuterConeAngle(0.65f); // 49.5 degrees

    getEditorComponent().registerField("Range and Cone", &_rangeAndCones, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
}

};