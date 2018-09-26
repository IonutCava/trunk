#include "stdafx.h"

#include "Headers/Light.h"
#include "Headers/LightPool.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"

namespace Divide {

Light::Light(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const F32 range, const LightType& type, LightPool& parentPool)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_LIGHT),
      _parentPool(parentPool),
      _type(type),
      _castsShadows(false),
      _directionAndConeChanged(false),
      _spotCosOuterConeAngle(0.0f)
{
    _shadowCameras.fill(nullptr);

    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i] = std::numeric_limits<F32>::max();
    }
    
    _shadowProperties._lightDetails.x = to_U32(type);
    setDiffuseColour(DefaultColours::WHITE);
    setRange(1.0f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    getEditorComponent().registerField("Position and Range", &_positionAndRange, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC4);
    getEditorComponent().registerField("Direction and Cone", &_directionAndCone, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC4);

    _enabled = true;
}

Light::~Light()
{
}

bool Light::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    for (U32 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowCameras[i] = Camera::createCamera(name() + "_shadowCamera_" + to_stringImpl(i), Camera::CameraType::FREE_FLY);

        _shadowCameras[i]->setMoveSpeedFactor(0.0f);
        _shadowCameras[i]->setTurnSpeedFactor(0.0f);
        _shadowCameras[i]->setFixedYawAxis(true);
    }
    if (_parentPool.addLight(*this)) {
        return SceneNode::load(onLoadCallback);
    }

    return false;
}

bool Light::unload() {
    _parentPool.removeLight(getGUID(), getLightType());
    for (U32 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        Camera::destroyCamera(_shadowCameras[i]);
    }
    _shadowCameras.fill(nullptr);

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode& sgn) {
    sgn.get<TransformComponent>()->setPosition(_positionAndRange.xyz());
    SceneNode::postLoad(sgn);
}

void Light::setDiffuseColour(const vec3<U8>& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

void Light::setRange(F32 range) {
    _positionAndRange.w = range;
}

void Light::setConeAngle(F32 newAngle) {
    _directionAndCone.w = newAngle;
    _directionAndConeChanged = true;
}

void Light::setSpotCosOuterConeAngle(F32 newCosAngle) {
    _spotCosOuterConeAngle = newCosAngle;
}

void Light::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    TransformComponent* tComp = sgn.get<TransformComponent>();
    vec3<F32> dir(tComp->getOrientation() * WORLD_Z_NEG_AXIS);
    dir.normalize();

    if (_directionAndCone.xyz() != dir) {
        _directionAndCone.xyz(dir);
        setBoundsChanged();
    }
    const vec3<F32>& pos = tComp->getPosition();
    if (pos != _positionAndRange.xyz()) {
        _positionAndRange.xyz(pos);
        setBoundsChanged();
    }

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

void Light::updateBoundsInternal() {
    if (_type == LightType::DIRECTIONAL) {
        vec3<F32> directionalLightPosition = _positionAndRange.xyz() * _positionAndRange.w * -1.0f;
        _boundingBox.set(directionalLightPosition - 10.0f, directionalLightPosition + 10.0f);
    } else {
        _boundingBox.set(vec3<F32>(-getRange()), vec3<F32>(getRange()));
        _boundingBox.multiply(0.5f);
    }

    if (_type == LightType::SPOT) {
        _boundingBox.multiply(0.5f);
    }
    SceneNode::updateBoundsInternal();
}

void Light::editorFieldChanged(EditorComponentField& field) {
    if (field._data == &_directionAndCone) {
        _directionAndConeChanged = true;
    }
}

};