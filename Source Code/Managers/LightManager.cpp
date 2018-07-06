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
      _previewShadowMaps(false),
      _currentShadowPass(0) {
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
    _cubeShadowLocation = 255;
    _normShadowLocation = 255;
    _arrayShadowLocation = 255;
    ParamHandler::getInstance().setParam<bool>("rendering.debug.showSplits",
                                               false);
}

LightManager::~LightManager() {
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

    STUBBED(
        "Replace light map bind slots with bindless textures! Max texture "
        "units is currently used! -Ionut!");

    REGISTER_FRAME_LISTENER(&(this->getInstance()), 2);

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

    _lightProperties.resize(Config::Lighting::MAX_LIGHTS_PER_SCENE);
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->Create(
        Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightProperties));
    _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->Bind(
        ShaderBufferLocation::LIGHT_NORMAL);

    _lightShadowProperties.resize(
        Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
    _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->Create(
        Config::Lighting::MAX_LIGHTS_PER_SCENE, sizeof(LightShadowProperties));
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

bool LightManager::addLight(Light* const light) {
    assert(light != nullptr);

    light->addShadowMapInfo(MemoryManager_NEW ShadowMapInfo(light));

    if (findLight(light->getGUID()) != std::end(_lights)) {
        Console::errorfn(Locale::get("ERROR_LIGHT_MANAGER_DUPLICATE"),
                         light->getGUID());
        return false;
    }

    vectorAlg::emplace_back(_lights, light->getGUID(), light);

    GET_ACTIVE_SCENE()->renderState().getCameraMgr().addNewCamera(
        light->getName(), light->shadowCamera());

    return true;
}

// try to remove any leftover lights
bool LightManager::removeLight(I64 lightGUID) {
    /// we can't remove a light if the light list is empty. That light has to
    /// exist somewhere!
    assert(!_lights.empty());

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
    _shadowMapsEnabled = false;
    s_shadowPassTimer->pause(!_shadowMapsEnabled);
}

void LightManager::updateResolution(I32 newWidth, I32 newHeight) {
    for (Light* light : _lights) {
        light->updateResolution(newWidth, newHeight);
    }

    _cachedResolution.set(newWidth, newHeight);
}

U8 LightManager::getShadowBindSlotOffset(ShadowSlotType type) {
    if (_cubeShadowLocation == _normShadowLocation &&
        _normShadowLocation == _arrayShadowLocation &&
        _arrayShadowLocation == 255) {
        U32 maxTextureStorage = ParamHandler::getInstance().getParam<I32>(
            "rendering.maxTextureSlots", 16);
        _cubeShadowLocation =
            maxTextureStorage -
            (Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * 3);
        _normShadowLocation =
            maxTextureStorage -
            (Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * 2);
        _arrayShadowLocation =
            maxTextureStorage -
            (Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE * 1);
    }
    switch (type) {
        default:
        case ShadowSlotType::SHADOW_SLOT_TYPE_NORMAL:
            return _normShadowLocation;
        case ShadowSlotType::SHADOW_SLOT_TYPE_CUBE:
            return _cubeShadowLocation;
        case ShadowSlotType::SHADOW_SLOT_TYPE_ARRAY:
            return _arrayShadowLocation;
    };
}

/// Check light properties for every light (this is bound to the camera change
/// listener group
/// Update only if needed. Get projection and view matrices if they changed
/// Also, search for the dominant light if any
void LightManager::onCameraChange() {
    for (Light* light : _lights) {
        light->onCameraChange();
    }
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightManager::framePreRenderEnded(const FrameEvent& evt) {
    if (!_shadowMapsEnabled) {
        return true;
    }

    Time::START_TIMER(s_shadowPassTimer);

    // Tell the engine that we are drawing to depth maps
    // set the current render stage to SHADOW
    RenderStage previousRS = GFX_DEVICE.setRenderStage(RenderStage::SHADOW);
    // generate shadowmaps for each light
    for (Light* light : _lights) {
        setCurrentLight(light);
        light->generateShadowMaps(GET_ACTIVE_SCENE()->renderState());
    }

    // Revert back to the previous stage
    GFX_DEVICE.setRenderStage(previousRS);

    Time::STOP_TIMER(s_shadowPassTimer);

    return true;
}

void LightManager::togglePreviewShadowMaps() {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled ||
        GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY) {
        return;
    }

    for (Light* light : _lights) {
        if (light->getShadowMapInfo()->getShadowMap()) {
            light->getShadowMapInfo()->getShadowMap()->togglePreviewShadowMaps(
                _previewShadowMaps);
        }
    }
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
    if (!light->castsShadows()) {
        return;
    }
    if (light->getShadowMapInfo()->getShadowMap()) {
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
    for (U8 i = 0;
         i < std::min((U32)_lights.size(),
                      Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
         ++i) {
        Light* lightLocal = getLight(i);
        assert(lightLocal);

        if (!lightLocal->castsShadows()) {
            continue;
        }
        ShadowMap* sm = lightLocal->getShadowMapInfo()->getShadowMap();
        if (sm) {
            sm->Bind(getShadowBindSlotOffset(lightLocal->getLightType()) + i);
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
        ShadowMapInfo* smi = light->getShadowMapInfo();
        // no shadow info;
        if (!smi) {
            continue;
        }
        ShadowMap* sm = smi->getShadowMap();
        // no shadow map;
        if (!sm) {
            continue;
        }
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
    U32 physicalIndex = to_uint(Light::PropertyType::PROPERTY_TYPE_PHYSICAL);
    U32 shadowIndex = to_uint(Light::PropertyType::PROPERTY_TYPE_SHADOW);
    U32 visualIndex = to_uint(Light::PropertyType::PROPERTY_TYPE_VISUAL);

    bool physycalFlag = false;
    bool shadowFlag = false;

    bool viewMatrixDirty = _viewMatrixCache != viewMatrix;
    if (viewMatrixDirty) {
        _viewMatrixCache.set(viewMatrix);
    }

    U32 i = 0, j = 0;
    U32 lightCount = std::min(static_cast<U32>(_lights.size()),
                              Config::Lighting::MAX_LIGHTS_PER_SCENE);
    U32 lightShadowCount =
        std::min(static_cast<U32>(_lights.size()),
                 Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);

    for (; i < lightCount; ++i) {
        Light* light = _lights[i];

        LightProperties& temp = _lightProperties[i];
        temp.set(light->getProperties());
        if (light->_dirty[physicalIndex] || viewMatrixDirty) {
            if (light->getLightType() == LightType::LIGHT_TYPE_DIRECTIONAL) {
                temp._position.set(
                    vec3<F32>(_viewMatrixCache *
                              vec4<F32>(temp._position.xyz(), 0.0f)),
                    temp._position.w);
            } else if (light->getLightType() == LightType::LIGHT_TYPE_SPOT) {
                temp._direction.set(
                    vec3<F32>(_viewMatrixCache *
                              vec4<F32>(temp._direction.xyz(), 0.0f)),
                    temp._direction.w);
            }

            light->_dirty[physicalIndex] = false;
            physycalFlag = true;
        }

        if (light->castsShadows() && j < lightShadowCount) {
            light->_dirty[shadowIndex] = false;
            LightShadowProperties& tempShadow = _lightShadowProperties[j];
            tempShadow.set(light->getShadowProperties());
            temp._options.z = j;
            ++j;
            shadowFlag = true;
        }

        light->_dirty[visualIndex] = false;
    }

    if (physycalFlag) {
        _lightShaderBuffer[to_uint(ShaderBufferType::NORMAL)]->UpdateData(
            0, lightCount, _lightProperties.data());
    }

    if (shadowFlag) {
        _lightShaderBuffer[to_uint(ShaderBufferType::SHADOW)]->UpdateData(
            0, lightShadowCount, _lightShadowProperties.data());
    }
}
};