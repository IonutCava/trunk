#include "Headers/Light.h"
#include "Headers/LightPool.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Light::Light(ResourceCache& parentCache, const stringImpl& name, const F32 range, const LightType& type, LightPool& parentPool)
    : SceneNode(parentCache, name, SceneNodeType::TYPE_LIGHT),
      _parentPool(parentPool),
      _type(type),
      _rangeChanged(true),
      _drawImpostor(false),
      _impostor(nullptr),
      _lightSGN(nullptr),
      _shadowMapInfo(nullptr),
      _shadowCamera(nullptr),
      _castsShadows(false),
      _spotPropertiesChanged(false),
      _spotCosOuterConeAngle(0.0f)
{
    if (type == LightType::DIRECTIONAL) {
        // 0,0,0 is not a valid direction for directional lights
        _positionAndRange.set(1.0f);
    }

    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i].set(std::numeric_limits<F32>::max());
    }
    
    setDiffuseColour(DefaultColours::WHITE());
    setRange(1.0f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    _enabled = true;
}

Light::~Light()
{
}

bool Light::load(const DELEGATE_CBK<void, Resource_ptr>& onLoadCallback) {
    _shadowCamera = Camera::createCamera(getName() + "_shadowCamera", Camera::CameraType::FREE_FLY);

    _shadowCamera->setMoveSpeedFactor(0.0f);
    _shadowCamera->setTurnSpeedFactor(0.0f);
    _shadowCamera->setFixedYawAxis(true);

    if (_parentPool.addLight(*this)) {
        return Resource::load(onLoadCallback);
    }

    return false;
}

bool Light::unload() {
    _parentPool.removeLight(getGUID(), getLightType());
    Camera::destroyCamera(_shadowCamera);
    removeShadowMapInfo();

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode& sgn) {
    // Hold a pointer to the light's location in the SceneGraph
    DIVIDE_ASSERT(
        _lightSGN == nullptr,
        "Light error: Lights can only be bound to a single SGN node!");

    _lightSGN = &sgn;
    _lightSGN->get<PhysicsComponent>()->setPosition(_positionAndRange.xyz());
    _lightSGN->get<BoundsComponent>()->lockBBTransforms(true);
    SceneNode::postLoad(sgn);
}

void Light::setDiffuseColour(const vec3<U8>& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

void Light::setRange(F32 range) {
    _positionAndRange.w = range;
    _rangeChanged = true;
}

void Light::setSpotAngle(F32 newAngle) {
    _spotProperties.w = newAngle;
    _spotPropertiesChanged = true;
}

void Light::setSpotCosOuterConeAngle(F32 newCosAngle) {
    _spotCosOuterConeAngle = newCosAngle;
}

void Light::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn, SceneState& sceneState) {
    vec3<F32> dir(_lightSGN->get<PhysicsComponent>()->getOrientation() * WORLD_Z_NEG_AXIS);
    dir.normalize();
    _spotProperties.xyz(dir);
    _positionAndRange.xyz(_lightSGN->get<PhysicsComponent>()->getPosition());
    setFlag(UpdateFlag::BOUNDS_CHANGED);

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

void Light::updateBoundsInternal(SceneGraphNode& sgn) {
    if (_type == LightType::DIRECTIONAL) {
        vec3<F32> directionalLightPosition =
            _positionAndRange.xyz() * 
            (to_const_float(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE) * -1.0f);
        _boundingBox.set(directionalLightPosition - 10.0f,
                         directionalLightPosition + 10.0f);
    } else {
        _boundingBox.set(vec3<F32>(-getRange()), vec3<F32>(getRange()));
        _boundingBox.multiply(0.5f);
    }

    if (_type == LightType::SPOT) {
        _boundingBox.multiply(0.5f);
    }
    SceneNode::updateBoundsInternal(sgn);
}

bool Light::onRender(const RenderStagePass& renderStagePass) {
    /*if (_type == LightType::DIRECTIONAL) {
        if (sceneState.playerState(0).overrideCamera() == nullptr) {
            sceneState.playerState(0).overrideCamera(_shadowCamera);
            GFXDevice::instance().debugDrawFrustum(&Camera::activeCamera()->getFrustum());
        }
    }*/

    if (!_drawImpostor) {
        return true;
    }

    if (!_impostor) {
        static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                      to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                      to_const_uint(SGNComponent::ComponentType::RENDERING);

        _impostor = CreateResource<ImpostorSphere>(_parentCache, ResourceDescriptor(_name + "_impostor"));
        _impostor->setRadius(_positionAndRange.w);
        _impostor->renderState().setDrawState(true);
        _impostorSGN = _lightSGN->addNode(_impostor, normalMask, PhysicsGroup::GROUP_IGNORE);
        _impostorSGN.lock()->setActive(true);
    }

    _impostorSGN.lock()->get<RenderingComponent>()->getMaterialInstance()->setDiffuse(getDiffuseColour());

    updateImpostor();

    return true;
}

void Light::updateImpostor() {
    if (_type == LightType::DIRECTIONAL) {
        return;
    }
    // Updating impostor range is expensive, so check if we need to
    if (_rangeChanged) {
        F32 range = getRange();
        if (_type == LightType::SPOT) {
            range *= 0.5f;
            // Spot light's bounding sphere extends from the middle of the light's range outwards,
            // touching the light's position on one end and the cone at the other
            // so we need to offest the impostor's position a bit
            PhysicsComponent* pComp = _impostorSGN.lock()->get<PhysicsComponent>();
            pComp->setPosition(getSpotDirection() * range);
        }
        _impostor->setRadius(range);
        _rangeChanged = false;
    }
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo) {
    MemoryManager::SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo() {
    MemoryManager::DELETE(_shadowMapInfo);
    return true;
}

void Light::validateOrCreateShadowMaps(GFXDevice& context, SceneRenderState& sceneRenderState) {
    _shadowMapInfo->createShadowMap(context, sceneRenderState, shadowCamera());
}

void Light::generateShadowMaps(GFXDevice& context, U32 passIdx) {
    ShadowMap* sm = _shadowMapInfo->getShadowMap();

    DIVIDE_ASSERT(sm != nullptr,
                  "Light::generateShadowMaps error: Shadow casting light "
                  "with no shadow map found!");

    _shadowProperties._arrayOffset.set(sm->getArrayOffset());
    sm->render(context, passIdx);

}

};