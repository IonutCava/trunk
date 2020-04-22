#include "stdafx.h"

#include "Headers/Light.h"
#include "Rendering/Lighting/Headers/LightPool.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace TypeUtil {
    const char* LightTypeToString(LightType lightType) noexcept {
        return Names::lightType[to_base(lightType)];
    }

    LightType StringToLightType(const stringImpl& name) {
        for (U8 i = 0; i < to_U8(LightType::COUNT); ++i) {
            if (strcmp(name.c_str(), Names::lightType[i]) == 0) {
                return static_cast<LightType>(i);
            }
        }

        return LightType::COUNT;
    }
};

Light::Light(SceneGraphNode& sgn, const F32 range, LightType type, LightPool& parentPool)
    : ECS::Event::IEventListener(sgn.sceneGraph().GetECSEngine()), 
      _parentPool(parentPool),
      _sgn(sgn),
      _type(type),
      _castsShadows(false),
      _shadowIndex(-1)
{
    _rangeAndCones.set(1.0f, 45.0f, 0.0f);

    for (U32 i = 0; i < _shadowCameras.size(); ++i) {
        _shadowCameras[i] = Camera::createCamera<FreeFlyCamera>(sgn.name() + "_shadowCamera_" + to_stringImpl(i));
        _shadowCameras[i]->updateFrustum();
    }
    if (!_parentPool.addLight(*this)) {
        //assert?
    }

    for (U8 i = 0; i < 6; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._lightPosition[i].w = std::numeric_limits<F32>::max();
    }
    
    _shadowProperties._lightDetails.x = to_F32(type);
    setDiffuseColour(FColour3(DefaultColours::WHITE));
    setRange(1.0f);

    const ECS::CustomEvent evt = {
        ECS::CustomEvent::Type::TransformUpdated,
        sgn.get<TransformComponent>(),
        to_U32(TransformType::ALL)
    };

    updateCache(evt);

    _enabled = true;
}

Light::~Light()
{
    UnregisterAllEventCallbacks();
    for (U32 i = 0; i < 6; ++i) {
        Camera::destroyCamera(_shadowCameras[i]);
    }
    _parentPool.removeLight(*this);
}

void Light::updateCache(const ECS::CustomEvent& data) {
    OPTICK_EVENT();

    TransformComponent* tComp = std::any_cast<TransformComponent*>(data._userData);
    assert(tComp != nullptr);

    if (_type != LightType::DIRECTIONAL && BitCompare(data._flag, to_U32(TransformType::TRANSLATION))) {
        _positionCache = tComp->getPosition();
    }

    if (_type != LightType::POINT && BitCompare(data._flag, to_U32(TransformType::ROTATION))) {
        _directionCache = Normalized(Rotate(WORLD_Z_NEG_AXIS, tComp->getOrientation()));
    }
}

void Light::setDiffuseColour(const UColour3& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

};