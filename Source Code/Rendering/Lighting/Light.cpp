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
      _rangeChanged(true),
      _drawImpostor(false),
      _impostor(nullptr),
      _shadowMapInfo(nullptr),
      _impostorSGN(nullptr),
      _castsShadows(false),
      _spotPropertiesChanged(false),
      _spotCosOuterConeAngle(0.0f)
{
    _shadowCameras.fill(nullptr);

    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i].set(std::numeric_limits<F32>::max());
    }
    
    setDiffuseColour(DefaultColours::WHITE);
    setRange(1.0f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

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
    removeShadowMapInfo();

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
    _rangeChanged = true;
}

void Light::setSpotAngle(F32 newAngle) {
    _spotProperties.w = newAngle;
    _spotPropertiesChanged = true;
}

void Light::setSpotCosOuterConeAngle(F32 newCosAngle) {
    _spotCosOuterConeAngle = newCosAngle;
}

void Light::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    TransformComponent* tComp = sgn.get<TransformComponent>();
    vec3<F32> dir(tComp->getOrientation() * WORLD_Z_NEG_AXIS);
    dir.normalize();

    if (_spotProperties != dir) {
        _spotProperties.xyz(dir);
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
        vec3<F32> directionalLightPosition =
            _positionAndRange.xyz() * 
            (to_U32(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE) * -1.0f);
        _boundingBox.set(directionalLightPosition - 10.0f,
                         directionalLightPosition + 10.0f);
    } else {
        _boundingBox.set(vec3<F32>(-getRange()), vec3<F32>(getRange()));
        _boundingBox.multiply(0.5f);
    }

    if (_type == LightType::SPOT) {
        _boundingBox.multiply(0.5f);
    }
    SceneNode::updateBoundsInternal();
}

bool Light::onRender(SceneGraphNode& sgn,
                     const SceneRenderState& sceneRenderState,
                     const RenderStagePass& renderStagePass) {
    ACKNOWLEDGE_UNUSED(sceneRenderState);

    if (!_drawImpostor) {
        return true;
    }

    if (!_impostor) {
        _impostor = CreateResource<ImpostorSphere>(_parentCache, ResourceDescriptor(_name + "_impostor"));
        _impostor->setRadius(_positionAndRange.w);
        _impostor->renderState().setDrawState(true);


        SceneGraphNodeDescriptor impostorDescriptor;
        impostorDescriptor._componentMask = to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS) | to_base(ComponentType::RENDERING);
        impostorDescriptor._node = _impostor;
        impostorDescriptor._physicsGroup = PhysicsGroup::GROUP_IGNORE;
        impostorDescriptor._isSelectable = false;
        impostorDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;

        _impostorSGN = sgn.addNode(impostorDescriptor);
        _impostorSGN->setActive(true);
    }

    _impostorSGN->get<RenderingComponent>()->getMaterialInstance()->setDiffuse(getDiffuseColour());

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
            TransformComponent* tComp = _impostorSGN->get<TransformComponent>();
            tComp->setPosition(getSpotDirection() * range);
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
    _shadowMapInfo->createShadowMap(context, sceneRenderState, _shadowCameras);
}

void Light::generateShadowMaps(U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    ShadowMap* sm = _shadowMapInfo->getShadowMap();

    DIVIDE_ASSERT(sm != nullptr,
                  "Light::generateShadowMaps error: Shadow casting light "
                  "with no shadow map found!");

    _shadowProperties._arrayOffset.set(sm->getArrayOffset());
    sm->render(passIdx, bufferInOut);

}

};