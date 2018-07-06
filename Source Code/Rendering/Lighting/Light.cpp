#include "Headers/Light.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Light::Light(const F32 range, const LightType& type)
    : SceneNode(SceneNodeType::TYPE_LIGHT),
      _type(type),
      _drawImpostor(false),
      _impostor(nullptr),
      _lightSGN(nullptr),
      _shadowMapInfo(nullptr),
      _castsShadows(false),
      _spotPropertiesChanged(false),
      _spotCosOuterConeAngle(0.0f)
{
    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i].set(-1.0f);
    }
    
    _shadowCamera = MemoryManager_NEW FreeFlyCamera();
    _shadowCamera->setMoveSpeedFactor(0.0f);
    _shadowCamera->setTurnSpeedFactor(0.0f);
    _shadowCamera->setFixedYawAxis(true);

    setDiffuseColor(DefaultColors::WHITE());
    setRange(1.0f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    _enabled = true;
}

Light::~Light()
{
}

bool Light::load(const stringImpl& name) {
    setName(name);
    return LightManager::getInstance().addLight(*this);
}

bool Light::unload() {
    LightManager::getInstance().removeLight(getGUID(), getLightType());

    removeShadowMapInfo();

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode& sgn) {
    // Hold a pointer to the light's location in the SceneGraph
    DIVIDE_ASSERT(
        _lightSGN == nullptr,
        "Light error: Lights can only be bound to a single SGN node!");

    _lightSGN = &sgn;
    _lightSGN->getComponent<PhysicsComponent>()->setPosition(_positionAndRange.xyz());
    SceneNode::postLoad(sgn);
}

void Light::onCameraUpdate(Camera& camera) {
    assert(_lightSGN != nullptr);

    if (!getEnabled()) {
        return;
    }
    
    if (_shadowMapInfo) {
        ShadowMap* sm = _shadowMapInfo->getShadowMap();
        if (sm) {
            sm->onCameraUpdate(camera);
        }
    }
}

void Light::setDiffuseColor(const vec3<U8>& newDiffuseColor) {
    _color.rgb(newDiffuseColor);
}

void Light::setRange(F32 range) {
    _positionAndRange.w = range;
}

void Light::setSpotDirection(const vec3<F32>& newDirection) {
    vec3<F32> newDirectionNormalized(newDirection);
    newDirectionNormalized.normalize();
    _spotProperties.xyz(newDirectionNormalized);
    _spotPropertiesChanged = true;
}

void Light::setSpotAngle(F32 newAngle) {
    _spotProperties.w = newAngle;
    _spotPropertiesChanged = true;
}

void Light::setSpotCosOuterConeAngle(F32 newCosAngle) {
    _spotCosOuterConeAngle = newCosAngle;
}

void Light::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn, SceneState& sceneState) {
    _positionAndRange.xyz(_lightSGN->getComponent<PhysicsComponent>()->getPosition());

    if (_type != LightType::DIRECTIONAL) {
        _lightSGN->updateBoundingBoxTransform(_lightSGN->getComponent<PhysicsComponent>()->getWorldMatrix());
        sgn.getBoundingBox().setComputed(false);
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Light::computeBoundingBox(SceneGraphNode& sgn) {
    if (_type == LightType::DIRECTIONAL) {
        vec3<F32> directionalLightPosition =
            _positionAndRange.xyz() * 
            to_const_float(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE) * -1.0f;
        sgn.getBoundingBox().set(directionalLightPosition - vec3<F32>(10),
                                 directionalLightPosition + vec3<F32>(10));
    } else if (_type == LightType::POINT) {
        sgn.getBoundingBox().set(vec3<F32>(-getRange()), vec3<F32>(getRange()));
    } else {
        sgn.getBoundingBox().set(vec3<F32>(-getRange()) * 0.5f, vec3<F32>(getRange()) * 0.5f);
    }

    return SceneNode::computeBoundingBox(sgn);
}

bool Light::isInView(const SceneRenderState& sceneRenderState,
                     SceneGraphNode& sgn,
                     Frustum::FrustCollision& collisionType,
                     const bool distanceCheck) {
    collisionType = _drawImpostor ? Frustum::FrustCollision::FRUSTUM_IN
                                  : Frustum::FrustCollision::FRUSTUM_OUT;
    return _drawImpostor;
}

bool Light::onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
    // The isInView call should stop impostor rendering if needed
    if (!_impostor) {
        _impostor = CreateResource<ImpostorSphere>(ResourceDescriptor(_name + "_impostor"));
        _impostor->setRadius(_positionAndRange.w);
        _impostor->renderState().setDrawState(true);
        _impostorSGN = _lightSGN->addNode(*_impostor);
        _impostorSGN.lock()->setActive(true);
    }

    Material* const impostorMaterialInst = _impostorSGN.lock()->getComponent<RenderingComponent>()->getMaterialInstance();
    impostorMaterialInst->setDiffuse(getDiffuseColor());

    // Updating impostor range is expensive, so check if we need to
    if (!COMPARE(getRange(), _impostor->getRadius())) {
        updateImpostor();
    }
    if (_type == LightType::SPOT && _spotPropertiesChanged) {
        updateImpostor();
        _spotPropertiesChanged = false;
    }
    return true;
}

void Light::updateImpostor() {
    if (_type == LightType::POINT) {
        _impostor->setRadius(getRange());
    } else if (_type == LightType::SPOT) {
        // Spot light's bounding sphere extends from the middle of the light's range outwards,
        // touching the light's position on one end and the cone at the other
        // so we need to offest the impostor's position a bit
        _impostor->setRadius(getRange() * 0.5f);
        PhysicsComponent* pComp = _impostorSGN.lock()->getComponent<PhysicsComponent>();
        pComp->setPosition(getSpotDirection() * (getRange() * 0.5f));
    }
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo) {
    MemoryManager::SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo() {
    MemoryManager::DELETE(_shadowMapInfo);
    return true;
}

void Light::validateOrCreateShadowMaps(SceneRenderState& sceneRenderState) {
    _shadowMapInfo->createShadowMap(sceneRenderState, shadowCamera());
}

void Light::generateShadowMaps(SceneRenderState& sceneRenderState) {
    ShadowMap* sm = _shadowMapInfo->getShadowMap();

    DIVIDE_ASSERT(sm != nullptr,
                  "Light::generateShadowMaps error: Shadow casting light "
                  "with no shadow map found!");

    _shadowProperties._arrayOffset.set(sm->getArrayOffset());
    sm->render(sceneRenderState);

}

};