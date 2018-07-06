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
    : FrameListener(),
      _init(false),
      _shadowMapsEnabled(true),
      _previewShadowMaps(false)
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
    // We bind shadow maps to the last available texture slots that the hardware
    // supports.
    // Starting offsets for each texture type is stored here
    _shadowLocation.fill(255);
    ParamHandler::getInstance().setParam<bool>(
        "rendering.debug.displayShadowDebugInfo", false);
}

LightManager::~LightManager()
{
    clear();
    Time::REMOVE_TIMER(s_shadowPassTimer);
    MemoryManager::DELETE(
        _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]);
    MemoryManager::DELETE(
        _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]);
}

void LightManager::init() {
    if (_init) {
        return;
    }

    STUBBED("Replace light map bind slots with bindless textures! "
            "Max texture units is currently used! -Ionut!");

    REGISTER_FRAME_LISTENER(&(getInstance()), 2);

    GFX_DEVICE.add2DRenderFunction(
        DELEGATE_BIND(&LightManager::previewShadowMaps, this, nullptr), 1);
    // NORMAL holds general info about the currently active
    // lights: position, color, etc.
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)] =
        GFX_DEVICE.newSB("dvd_LightBlock");
    // SHADOWS holds info about the currently active shadow
    // casting lights:
    // ViewProjection Matrices, View Space Position, etc
    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)] =
        GFX_DEVICE.newSB("dvd_ShadowBlock");

    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->Create(
        Config::Lighting::MAX_LIGHTS_PER_SCENE, 1, sizeof(LightProperties));
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->Bind(
        ShaderBufferLocation::LIGHT_NORMAL);

    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->Create(
        Config::Lighting::MAX_LIGHTS_PER_SCENE, 1, sizeof(LightShadowProperties));
    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->Bind(
        ShaderBufferLocation::LIGHT_SHADOW);

    _cachedResolution.set(
        GFX_DEVICE.getRenderTarget(GFXDevice::RenderTarget::SCREEN)
            ->getResolution());
    _init = true;
}

bool LightManager::clear() {
    if (!_init) {
        return true;
    }

    UNREGISTER_FRAME_LISTENER(&(this->getInstance()));

    // Lights are removed by the sceneGraph
    // (range_based for-loops will fail due to iterator invalidation
    size_t lightCount = _lights.size();
    for (size_t i = 0; i < lightCount; i++) {
        // in case we had some light hanging
        RemoveResource(_lights[i]);
    }

    _lights.clear();

    _init = false;

    return _lights.empty();
}

bool LightManager::addLight(Light& light) {
    light.addShadowMapInfo(MemoryManager_NEW ShadowMapInfo(&light));

    if (findLight(light.getGUID()) != std::end(_lights)) {
        Console::errorfn(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"),
                         light.getGUID());
        return false;
    }

    vectorAlg::emplace_back(_lights, light.getGUID(), &light);

    GET_ACTIVE_SCENE().renderState().getCameraMgr().addNewCamera(
        light.getName(), light.shadowCamera());

    return true;
}

// try to remove any leftover lights
bool LightManager::removeLight(I64 lightGUID) {
    Light::LightList::const_iterator it = findLight(lightGUID);

    if (it == std::end(_lights)) {
        Console::errorfn(Locale::get("ERROR_LIGHT_MANAGER_REMOVE_LIGHT"),
                         lightGUID);
        return false;
    }

    _lights.erase(it);  // remove it from the map
    return true;
}

void LightManager::idle() {
    _shadowMapsEnabled =
        ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");

    s_shadowPassTimer->pause(!_shadowMapsEnabled);
}

void LightManager::updateResolution(I32 newWidth, I32 newHeight) {
    for (Light* light : _lights) {
        light->updateResolution(newWidth, newHeight);
    }

    _cachedResolution.set(newWidth, newHeight);
}

U8 LightManager::getShadowBindSlotOffset(ShadowType type) {
    if (_shadowLocation.front() == 255) {
        const I32 maxTextureStorage = ParamHandler::getInstance().getParam<I32>(
            "rendering.maxTextureSlots", 16);
        const U32 maxShadowSources =
            Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE;
        _shadowLocation.fill(maxTextureStorage);
        _shadowLocation[to_uint(ShadowType::CUBEMAP)] -= (maxShadowSources * 3);
        _shadowLocation[to_uint(ShadowType::SINGLE)]  -= (maxShadowSources * 2);
        _shadowLocation[to_uint(ShadowType::LAYERED)] -= (maxShadowSources * 1);
    }

    return _shadowLocation[to_uint(type)];
}

/// Check light properties for every light (this is bound to the camera change
/// listener group
/// Update only if needed. Get projection and view matrices if they changed
/// Also, search for the dominant light if any
void LightManager::onCameraUpdate(Camera& camera) {
    for (Light* light : _lights) {
        light->onCameraUpdate(camera);
    }
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightManager::framePreRenderEnded(const FrameEvent& evt) {
    if (!_shadowMapsEnabled) {
        return true;
    }

    Time::ScopedTimer timer(*s_shadowPassTimer);
    // Tell the engine that we are drawing to depth maps
    // set the current render stage to SHADOW
    RenderStage previousRS = GFX_DEVICE.setRenderStage(RenderStage::SHADOW);
    // generate shadowmaps for each light
    for (Light* light : _lights) {
        light->generateShadowMaps(GET_ACTIVE_SCENE().renderState());
    }

    // Revert back to the previous stage
    GFX_DEVICE.setRenderStage(previousRS);

    return true;
}

void LightManager::togglePreviewShadowMaps() {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled ||
        GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        ParamHandler::getInstance().setParam(
            "rendering.debug.displayShadowDebugInfo", false);
        return;
    }

    ParamHandler::getInstance().setParam(
        "rendering.debug.displayShadowDebugInfo", _previewShadowMaps);
}

void LightManager::previewShadowMaps(Light* light) {
#ifdef _DEBUG
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !_previewShadowMaps || _lights.empty() ||
        GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        return;
    }

    if (!light) {
        light = _lights.front();
    }

    if (light->castsShadows()) {
        assert(light->getShadowMapInfo()->getShadowMap() != nullptr);
        light->getShadowMapInfo()->getShadowMap()->previewShadowMaps();
    }
#endif
}

// If we have computed shadowmaps, bind them before rendering any geometry;
void LightManager::bindDepthMaps() {
    // Skip applying shadows if we are rendering to depth map, or we have
    // shadows disabled
    if (!_shadowMapsEnabled) {
        return;
    }
    Light::LightList& lights = getLights();

    U16 idx = 0;
    for (Light* light : lights) {
        if (light->castsShadows()) {
            ShadowMap* sm = light->getShadowMapInfo()->getShadowMap();
            DIVIDE_ASSERT(sm != nullptr,
                "LightManager::bindDepthMaps error: Shadow casting light "
                "with no shadow map found!");
            sm->Bind(getShadowBindSlotOffset(light->getLightType()) + idx++);

            if (idx >= Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE) {
                break;
            }
        }
    }
}

bool LightManager::shadowMappingEnabled() const {
    if (!_shadowMapsEnabled) {
        return false;
    }

    for (Light* const light : _lights) {
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

    return false;
}

Light* LightManager::getLight(I64 lightGUID) {
    Light::LightList::const_iterator it = findLight(lightGUID);
    assert(it != std::end(_lights));
    return *it;
}

void LightManager::updateAndUploadLightData(const mat4<F32>& viewMatrix) {
    bool viewMatrixDirty = _viewMatrixCache != viewMatrix;
    if (viewMatrixDirty) {
        _viewMatrixCache.set(viewMatrix);
    }

    U32 lightCount = std::min(to_uint(_lights.size()),
                              Config::Lighting::MAX_LIGHTS_PER_SCENE);
    U32 lightShadowCount =
        std::min(to_uint(_lights.size()),
                 Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);

    vectorImpl<LightProperties> lightProperties;
    lightProperties.reserve(_lights.size());
    vectorImpl<LightShadowProperties> lightShadowProperties;
    lightShadowProperties.reserve(lightShadowCount);

    for (Light* light : _lights) {
        lightProperties.push_back(light->getProperties());
        LightProperties& temp = lightProperties.back();

        if (light->_placementDirty || viewMatrixDirty) {
            if (light->getLightType() == LightType::DIRECTIONAL) {
                temp._position.set(
                    vec3<F32>(_viewMatrixCache *
                              vec4<F32>(temp._position.xyz(), 0.0f)),
                    temp._position.w);
            } else if (light->getLightType() == LightType::SPOT) {
                temp._direction.set(
                    vec3<F32>(_viewMatrixCache *
                              vec4<F32>(temp._direction.xyz(), 0.0f)),
                    temp._direction.w);
            }

            light->_placementDirty = false;
        }

        if (light->castsShadows() &&
            lightShadowProperties.size() < lightShadowCount) {

            temp._options.z = to_int(lightShadowProperties.size());
            lightShadowProperties.push_back(light->getShadowProperties());
        }
    }

    if (!lightProperties.empty()) {
        _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->UpdateData(
            0, lightProperties.size(), lightProperties.data());
    }

    if (!lightShadowProperties.empty()) {
        _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->UpdateData(
            0, lightShadowProperties.size(), lightShadowProperties.data());
    }
}
};