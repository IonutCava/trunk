#include "stdafx.h"

#include "Headers/Light.h"
#include "Rendering/Lighting/Headers/LightPool.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"

namespace Divide {

namespace {
    ComponentType CTypeFromLType(LightType lightType) {
        if (lightType == LightType::DIRECTIONAL)
            return ComponentType::DIRECTIONAL_LIGHT;

        if (lightType == LightType::POINT)
            return ComponentType::POINT_LIGHT;

        return ComponentType::SPOT_LIGHT;
    }
};

template<typename T>
Light<T>::Light<T>(SceneGraphNode& sgn, const F32 range, const LightType& type, LightPool& parentPool)
    : SGNComponent<T>(sgn, CTypeFromLType(type)),
      _parentPool(parentPool),
      _sgn(sgn),
      _type(type),
      _castsShadows(false)
{
    _rangeAndCones.set(1.0f, 45.0f, 0.0f);

    _shadowCameras.fill(nullptr);
    for (U32 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowCameras[i] = Camera::createCamera(sgn.name() + "_shadowCamera_" + to_stringImpl(i), Camera::CameraType::FREE_FLY);

        _shadowCameras[i]->setMoveSpeedFactor(0.0f);
        _shadowCameras[i]->setTurnSpeedFactor(0.0f);
        _shadowCameras[i]->setFixedYawAxis(true);
    }
    if (!_parentPool.addLight(*this)) {
        //assert?
    }

    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i] = std::numeric_limits<F32>::max();
    }
    
    _shadowProperties._lightDetails.x = to_U32(type);
    setDiffuseColour(DefaultColours::WHITE);
    setRange(1.0f);

    _enabled = true;
}

template<typename T>
Light<T>::~Light<T>()
{
    for (U32 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        Camera::destroyCamera(_shadowCameras[i]);
    }
    _parentPool.removeLight(*this);
}

template<typename T>
void Light<T>::setDiffuseColour(const vec3<U8>& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

};