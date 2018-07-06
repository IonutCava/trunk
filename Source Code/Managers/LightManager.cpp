#include "Headers/LightManager.h"
#include "Headers/SceneManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ProfileTimer.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

Time::ProfileTimer* s_shadowPassTimer = nullptr;

LightManager::LightManager()
    : _init(false),
      _shadowMapsEnabled(true),
      _previewShadowMaps(false),
      _currentShadowCastingLight(nullptr),
      _activeLightCount(0)
{
    // shadowPassTimer is used to measure the CPU-duration of shadow map
    // generation step
    s_shadowPassTimer = Time::ADD_TIMER("ShadowPassTimer");
    // NORMAL holds general info about the currently active
    // lights: position, color, etc.
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)] = nullptr;
    // SHADOWS holds info about the currently active shadow
    // casting lights:
    // ViewProjection Matrices, View Space Position, etc
    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)] = nullptr;
    const U8 startOffset = to_ubyte(ShaderProgram::TextureUsage::COUNT);
    _shadowLocation[to_uint(ShadowType::CUBEMAP)] = startOffset + 1;
    _shadowLocation[to_uint(ShadowType::SINGLE)] = startOffset + 2;
    _shadowLocation[to_uint(ShadowType::LAYERED)] = startOffset + 3;

    ParamHandler::getInstance().setParam<bool>(_ID("rendering.debug.displayShadowDebugInfo"), false);
}

LightManager::~LightManager()
{
    clear();
    Time::REMOVE_TIMER(s_shadowPassTimer);
    MemoryManager::DELETE(_lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]);
    MemoryManager::DELETE(_lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]);
    ShadowMap::clearShadowMaps();
}

void LightManager::init() {
    if (_init) {
        return;
    }

    ShadowMap::initShadowMaps();

    GFX_DEVICE.add2DRenderFunction(
        DELEGATE_BIND(&LightManager::previewShadowMaps, this, nullptr), 1);
    // NORMAL holds general info about the currently active
    // lights: position, color, etc.
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)] = GFX_DEVICE.newSB("dvd_LightBlock",
                                                                             1,
                                                                             true,
                                                                             false,
                                                                             BufferUpdateFrequency::OCASSIONAL);
    // SHADOWS holds info about the currently active shadow
    // casting lights:
    // ViewProjection Matrices, View Space Position, etc
    // Should be SSBO (not UBO) to use std430 alignment. Structures should not be padded
    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)] = GFX_DEVICE.newSB("dvd_ShadowBlock",
                                                                             1,
                                                                             true,
                                                                             false,
                                                                             BufferUpdateFrequency::OCASSIONAL);

    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]
        ->create(Config::Lighting::MAX_POSSIBLE_LIGHTS, sizeof(LightProperties));

    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]
        ->create(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS, sizeof(Light::ShadowProperties));

    _init = true;
}

bool LightManager::clear() {
    if (!_init) {
        return true;
    }

    bool success = true;
    for (Light::LightList& lightList : _lights) {
        // Lights are removed by the sceneGraph
        // (range_based for-loops will fail due to iterator invalidation
        vectorAlg::vecSize lightCount = lightList.size();
        for (U32 i = 0; i < lightCount; ++i) {
            // in case we had some light hanging
            if (!RemoveResource(lightList[i])) {
                success = false;
            }
        }
        lightList.clear();
    }

    if (success) {
        _init = false;
    }

    return success;
}

bool LightManager::addLight(Light& light) {
    light.addShadowMapInfo(MemoryManager_NEW ShadowMapInfo(&light));

    const LightType type = light.getLightType();
    const U32 lightTypeIdx = to_const_uint(type);

    if (findLight(light.getGUID(), type) != std::end(_lights[lightTypeIdx])) {

        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_MANAGER_DUPLICATE")),
                         light.getGUID());
        return false;
    }

    vectorAlg::emplace_back(_lights[lightTypeIdx], &light);

    GET_ACTIVE_SCENE().renderState().getCameraMgr().addNewCamera(
        light.getName(), light.shadowCamera());

    return true;
}

// try to remove any leftover lights
bool LightManager::removeLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);

    if (it == std::end(_lights[to_uint(type)])) {
        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_MANAGER_REMOVE_LIGHT")),
                         lightGUID);
        return false;
    }

    _lights[to_uint(type)].erase(it);  // remove it from the map
    return true;
}

void LightManager::idle() {
    _shadowMapsEnabled =
        ParamHandler::getInstance().getParam<bool>(_ID("rendering.enableShadows"));

    s_shadowPassTimer->pause(!_shadowMapsEnabled);
}

U8 LightManager::getShadowBindSlotOffset(ShadowType type) {
    return _shadowLocation[to_uint(type)];
}

/// Check light properties for every light (this is bound to the camera change
/// listener group
/// Update only if needed. Get projection and view matrices if they changed
/// Also, search for the dominant light if any
void LightManager::onCameraUpdate(Camera& camera) {
    for (Light::LightList& lights : _lights) {
        for (Light* light : lights) {
            light->onCameraUpdate(camera);
        }
    }
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightManager::generateShadowMaps() {
    if (!_shadowMapsEnabled) {
        return true;
    }
    ShadowMap::clearShadowMapBuffers();
    Time::ScopedTimer timer(*s_shadowPassTimer);
    SceneRenderState& state = GET_ACTIVE_SCENE().renderState();
    // generate shadowmaps for each light
    for (Light::LightList& lights : _lights) {
        for (Light* light : lights) {
            if (light->castsShadows()) {
                _currentShadowCastingLight = light;
                light->validateOrCreateShadowMaps(state);
                light->generateShadowMaps(state);
            }
        }
    }

    _currentShadowCastingLight = nullptr;
    return true;
}

void LightManager::togglePreviewShadowMaps() {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled ||
        GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        ParamHandler::getInstance().setParam(
            _ID("rendering.debug.displayShadowDebugInfo"), false);
        return;
    }

    ParamHandler::getInstance().setParam(
        _ID("rendering.debug.displayShadowDebugInfo"), _previewShadowMaps);
}

void LightManager::previewShadowMaps(Light* light) {
#ifdef _DEBUG
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !_previewShadowMaps || _lights.empty() ||
        GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        return;
    }

    // If no light is specified, get the first directional light
    if (!light) {
        Light::LightList& dirLights = _lights[to_uint(LightType::DIRECTIONAL)];
        if (!dirLights.empty()) {
            light = dirLights.front();
        }
    }

    // If no light is specified and there are no directional lights available,
    // grab the first light we can find
    if (!light) {
        for (Light::LightList& lights : _lights) {
            if (!lights.empty()) {
                light = lights.front();
                if (light->castsShadows()) {
                    break;
                }
            }
        }
    }

    if (light && light->castsShadows()) {
        assert(light->getShadowMapInfo()->getShadowMap() != nullptr);
        light->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
    }
#endif
}

// If we have computed shadowmaps, bind them before rendering any geometry;
void LightManager::bindShadowMaps() {
    // Skip applying shadows if we are rendering to depth map, or we have
    // shadows disabled
    if (!_shadowMapsEnabled) {
        return;
    }

    ShadowMap::bindShadowMaps();
}

bool LightManager::shadowMappingEnabled() const {
    if (!_shadowMapsEnabled) {
        return false;
    }

    for (const Light::LightList& lights : _lights) {
        for (Light* const light : lights) {
            if (!light->castsShadows()) {
                continue;
            }
            ShadowMap* sm = light->getShadowMapInfo()->getShadowMap();
            // no shadow map;
            assert(sm != nullptr);

            if (sm->getDepthMap()) {
                return true;
            }
        }
    }

    return false;
}

Light* LightManager::getLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);
    assert(it != std::end(_lights[to_uint(type)]));

    return *it;
}

void LightManager::updateAndUploadLightData(const mat4<F32>& viewMatrix) {
    U32 lightShadowCount = std::min(to_uint(_lights.size()), Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

    _activeLightCount = 0;
    U32 lightShadowPropertiesCount = 0;
    for(Light::LightList& lights : _lights) {
        for (Light* light : lights) {
            if (!light->getEnabled()) {
                continue;
            }

            if (_activeLightCount >= Config::Lighting::MAX_POSSIBLE_LIGHTS) {
                break;
            }

            LightProperties& temp = _lightProperties[_activeLightCount];
            temp._diffuse.set(light->getDiffuseColor());
            temp._position.set((viewMatrix * vec4<F32>(light->getPosition(), 0.0f)).xyz(), light->getRange());
            temp._direction.set((viewMatrix * vec4<F32>(light->getSpotDirection(), 0.0f)).xyz(), light->getSpotAngle());
            temp._options.x = to_uint(light->getLightType());
            temp._options.y = light->castsShadows();
            if (light->castsShadows() && lightShadowPropertiesCount < lightShadowCount) {

                temp._options.z = to_int(lightShadowPropertiesCount);
                _lightShadowProperties[lightShadowPropertiesCount++] = light->getShadowProperties();
            }

            _activeLightCount++;
        }

    }

    if (_activeLightCount > 0) {
        _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->updateData(0, _activeLightCount, _lightProperties.data());
        _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->bind(ShaderBufferLocation::LIGHT_NORMAL);
    }

    if (lightShadowPropertiesCount > 0) {
        _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->updateData(0, lightShadowPropertiesCount, _lightShadowProperties.data());
        _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->bind(ShaderBufferLocation::LIGHT_SHADOW);
    }
}
};
