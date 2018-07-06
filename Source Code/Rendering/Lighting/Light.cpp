#include "Headers/Light.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Light::Light(const F32 range, const LightType& type)
    : SceneNode(TYPE_LIGHT),
      _type(type),
      _drawImpostor(false),
      _updateLightBB(false),
      _lightSGN(nullptr),
      _impostor(nullptr),
      _resolutionFactor(1),
      _impostorSGN(nullptr),
      _par(ParamHandler::getInstance()),
      _shadowMapInfo(nullptr),
      _score(0.0f) {
    // All lights default to fully dynamic for now.
    setLightMode(LIGHT_MODE_MOVABLE);
    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
    }
    _shadowProperties._floatValues.set(-1.0f);

    _shadowCamera = MemoryManager_NEW FreeFlyCamera();
    _shadowCamera->setMoveSpeedFactor(0.0f);
    _shadowCamera->setTurnSpeedFactor(0.0f);
    _shadowCamera->setFixedYawAxis(true);

    _properties._diffuse.set(DefaultColors::WHITE());
    _properties._position.w = 35.0f;
    _properties._options.x = getLightTypeValue();
    _properties._options.y = 0;
    _properties._direction.w = 0.0f;
    _properties._attenuation =
        vec4<F32>(1.0f, 0.07f, 0.017f, 1.0f);  // constAtt, linearAtt, quadAtt

    memset(_dirty, true, PropertyType_PLACEHOLDER * sizeof(bool));
    _enabled = true;
    _renderState.addToDrawExclusionMask(DEPTH_STAGE);
}

Light::~Light() { unload(); }

bool Light::load(const stringImpl& name) {
    setName(name);
    setShadowMappingCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
                                           &SceneManager::getInstance()));
    assert(LightManager::hasInstance());
    return LightManager::getInstance().addLight(this);
}

bool Light::unload() {
    if (getState() != RES_LOADED && getState() != RES_LOADING) {
        return true;
    }

    // DELETE(_shadowCamera); <-- deleted by the camera manager
    LightManager::getInstance().removeLight(getGUID());

    removeShadowMapInfo();

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode* const sgn) {
    // Hold a pointer to the light's location in the SceneGraph
    DIVIDE_ASSERT(
        _lightSGN == nullptr,
        "Light error: Lights can only be bound to a single SGN node!");

    _lightSGN = sgn;
    _updateLightBB = true;

    SceneNode::postLoad(sgn);
}

void Light::onCameraChange() {
    assert(_lightSGN != nullptr);
    if (!getEnabled()) {
        return;
    }

    _dirty[PROPERTY_TYPE_PHYSICAL] = true;

    if (_type != LIGHT_TYPE_DIRECTIONAL) {
        if (_mode == LIGHT_MODE_MOVABLE) {
            const vec3<F32>& newPosition =
                _lightSGN->getComponent<PhysicsComponent>()->getPosition();
            if (_properties._position.xyz() != newPosition) {
                _properties._position.xyz(newPosition);
            }
        } else {
            _lightSGN->getComponent<PhysicsComponent>()->setPosition(
                _properties._position.xyz());
        }

        if (_updateLightBB) {
            _lightSGN->updateBoundingBoxTransform(
                _lightSGN->getComponent<PhysicsComponent>()->getWorldMatrix());
        }
    }

    _updateLightBB = false;
}

void Light::setPosition(const vec3<F32>& newPosition) {
    // Togglable lights can't be moved.
    if (_mode == LIGHT_MODE_TOGGLABLE) {
        Console::errorfn(Locale::get("WARNING_ILLEGAL_PROPERTY"), getGUID(),
                         "Light_Togglable", "LIGHT_POSITION");
        return;
    }

    _properties._position.xyz(newPosition);

    if (_mode == LIGHT_MODE_MOVABLE) {
        _lightSGN->getComponent<PhysicsComponent>()->setPosition(newPosition);
    }
    _updateLightBB = true;

    _dirty[PROPERTY_TYPE_PHYSICAL] = true;
}

void Light::setDiffuseColor(const vec3<F32>& newDiffuseColor) {
    _properties._diffuse.set(newDiffuseColor, _properties._diffuse.a);
    _dirty[PROPERTY_TYPE_VISUAL] = true;
}

void Light::setRange(F32 range) {
    _properties._attenuation.w = range;
    _dirty[PROPERTY_TYPE_PHYSICAL] = true;
}

void Light::setDirection(const vec3<F32>& newDirection) {
    // Togglable lights can't be moved.
    if (_mode == LIGHT_MODE_TOGGLABLE) {
        Console::errorfn(Locale::get("WARNING_ILLEGAL_PROPERTY"), getGUID(),
                         "Light_Togglable", "LIGHT_DIRECTION");
        return;
    }
    vec3<F32> newDirectionNormalized(newDirection);
    newDirectionNormalized.normalize();

    if (_type == LIGHT_TYPE_SPOT) {
        _properties._direction.xyz(newDirectionNormalized);
    } else {
        _properties._position.xyz(newDirectionNormalized);
    }

    _updateLightBB = true;
    _dirty[PROPERTY_TYPE_PHYSICAL] = true;
}

void Light::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn,
                        SceneState& sceneState) {
    if (_type == LIGHT_TYPE_DIRECTIONAL) {
        return;
    }

    // Check if range changed
    if (FLOAT_COMPARE(getRange(), sgn->getBoundingBoxConst().getMax().x)) {
        return;
    }

    sgn->getBoundingBox().setComputed(false);

    _updateLightBB = true;

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Light::computeBoundingBox(SceneGraphNode* const sgn) {
    if (_type == LIGHT_TYPE_DIRECTIONAL) {
        vec3<F32> directionalLightPosition =
            _properties._position.xyz() *
            Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE * -1.0f;
        sgn->getBoundingBox().set(directionalLightPosition - vec3<F32>(10),
                                  directionalLightPosition + vec3<F32>(10));
    } else {
        sgn->getBoundingBox().set(vec3<F32>(-getRange()),
                                  vec3<F32>(getRange()));
    }

    _updateLightBB = true;

    return SceneNode::computeBoundingBox(sgn);
}

bool Light::isInView(const SceneRenderState& sceneRenderState,
                     SceneGraphNode* const sgn, const bool distanceCheck) {
    return ((_impostorSGN != nullptr) && _drawImpostor);
}

void Light::render(SceneGraphNode* const sgn,
                   const SceneRenderState& sceneRenderState,
                   const RenderStage& currentRenderStage) {
    // The isInView call should stop impostor rendering if needed
    if (!_impostor) {
        _impostor =
            CreateResource<Impostor>(ResourceDescriptor(_name + "_impostor"));
        _impostor->setRadius(_properties._attenuation.w);
        _impostor->renderState().setDrawState(true);
        _lightSGN->addNode(_impostor)->setActive(true);
    }
    SceneGraphNode* impostorSGN = std::begin(_lightSGN->getChildren())->second;
    Material* const impostorMaterialInst =
        impostorSGN->getComponent<RenderingComponent>()->getMaterialInstance();
    impostorMaterialInst->setDiffuse(getDiffuseColor());
    impostorMaterialInst->setAmbient(getDiffuseColor());

    // Updating impostor range is expensive, so check if we need to
    if (!FLOAT_COMPARE(getRange(), _impostor->getRadius())) {
        _impostor->setRadius(getRange());
    }
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo) {
    MemoryManager::SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo() {
    MemoryManager::DELETE(_shadowMapInfo);
    return true;
}
void Light::updateResolution(I32 newWidth, I32 newHeight) {
    ShadowMap* sm = _shadowMapInfo->getShadowMap();

    if (!sm) {
        return;
    }

    sm->updateResolution(newWidth, newHeight);
}

void Light::generateShadowMaps(SceneRenderState& sceneRenderState) {
    ShadowMap* sm =
        _shadowMapInfo->getOrCreateShadowMap(sceneRenderState, _shadowCamera);

    if (!sm) {
        return;
    }

    sm->render(sceneRenderState, _callback);
    sm->postRender();

    _dirty[PROPERTY_TYPE_SHADOW] = true;
}

void Light::setLightMode(const LightMode& mode) { _mode = mode; }
};