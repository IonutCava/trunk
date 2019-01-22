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

Light::Light(SceneGraphNode& sgn, const F32 range, LightType type, LightPool& parentPool)
    : _parentPool(parentPool),
      _sgn(sgn),
      _type(type),
      _castsShadows(false)
{
    _rangeAndCones.set(1.0f, 45.0f, 0.0f);

    _shadowCameras.fill(nullptr);
    for (U32 i = 0; i < 6; ++i) {
        _shadowCameras[i] = Camera::createCamera(sgn.name() + "_shadowCamera_" + to_stringImpl(i), Camera::CameraType::FREE_FLY);

        _shadowCameras[i]->setMoveSpeedFactor(0.0f);
        _shadowCameras[i]->setTurnSpeedFactor(0.0f);
        _shadowCameras[i]->setFixedYawAxis(true);
    }
    if (!_parentPool.addLight(*this)) {
        //assert?
    }

    for (U8 i = 0; i < 6; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._lightPosition[i].w = std::numeric_limits<F32>::max();
    }
    
    _shadowProperties._lightDetails.x = to_U32(type);
    setDiffuseColour(DefaultColours::WHITE);
    setRange(1.0f);

    _enabled = true;
}

Light::~Light()
{
    for (U32 i = 0; i < 6; ++i) {
        Camera::destroyCamera(_shadowCameras[i]);
    }
    _parentPool.removeLight(*this);
}

void Light::setDiffuseColour(const vec3<U8>& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

vec3<F32> Light::getPosition() const {
    TransformComponent* lightTransform = getSGN().get<TransformComponent>();
    assert(lightTransform != nullptr);

    return lightTransform->getPosition();
}

vec3<F32> Light::getDirection() const {
    TransformComponent* lightTransform = getSGN().get<TransformComponent>();
    assert(lightTransform != nullptr);
    return Normalize(Rotate(WORLD_Z_NEG_AXIS, lightTransform->getOrientation()));
}
};