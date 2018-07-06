#include "Headers/LightPool.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Rendering/Lighting/ShadowMapping/Headers/ShadowMap.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

std::array<U8, to_const_uint(ShadowType::COUNT)> LightPool::_shadowLocation = { {
    to_const_ubyte(ShaderProgram::TextureUsage::COUNT) + 2u, //Single
    to_const_ubyte(ShaderProgram::TextureUsage::COUNT) + 3u, //Layered
    to_const_ubyte(ShaderProgram::TextureUsage::COUNT) + 1u  //Cubemap
}};

Light* LightPool::_currentShadowCastingLight = nullptr;

bool LightPool::_previewShadowMaps = false;
bool LightPool::_shadowMapsEnabled = true;

LightPool::LightPool(Scene& parentScene, GFXDevice& context)
    : SceneComponent(parentScene),
      _context(context),
      _previewShadowMapsCBK(DELEGATE_BIND(&LightPool::previewShadowMaps, this, nullptr)),
      _init(false),
      _buffersUpdated(false),
      _lightImpostorShader(nullptr),
      _lightIconsTexture(nullptr),
     // shadowPassTimer is used to measure the CPU-duration of shadow map generation step
     _shadowPassTimer(Time::ADD_TIMER("Shadow Pass Timer"))
{
    _activeLightCount.fill(0);
    _lightTypeState.fill(true);
    // NORMAL holds general info about the currently active
    // lights: position, colour, etc.
    _lightShaderBuffer[to_const_uint(ShaderBufferType::NORMAL)] = nullptr;
    // SHADOWS holds info about the currently active shadow
    // casting lights:
    // ViewProjection Matrices, View Space Position, etc
    _lightShaderBuffer[to_const_uint(ShaderBufferType::SHADOW)] = nullptr;

    ParamHandler::instance().setParam<bool>(_ID("rendering.debug.displayShadowDebugInfo"), false);

    _sortedLights.reserve(Config::Lighting::MAX_POSSIBLE_LIGHTS);
    _sortedShadowCastingLights.reserve(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
    _sortedLightProperties.reserve(Config::Lighting::MAX_POSSIBLE_LIGHTS);
    _sortedShadowProperties.reserve(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

    init();
}

LightPool::~LightPool()
{
    clear();
}

void LightPool::init() {
    if (_init) {
        return;
    }

    _context.add2DRenderFunction(_previewShadowMapsCBK, 1);

    ShaderBufferParams params;
    params._primitiveCount = Config::Lighting::MAX_POSSIBLE_LIGHTS;
    params._primitiveSizeInBytes = sizeof(LightProperties);
    params._ringBufferLength = 1;
    params._unbound = true;
    params._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    // NORMAL holds general info about the currently active lights: position, colour, etc.
    _lightShaderBuffer[to_const_uint(ShaderBufferType::NORMAL)] = _context.newSB(params);

    // SHADOWS holds info about the currently active shadow casting lights:
    // ViewProjection Matrices, View Space Position, etc
    // Should be SSBO (not UBO) to use std430 alignment. Structures should not be padded
    params._primitiveCount = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
    params._primitiveSizeInBytes = sizeof(Light::ShadowProperties);
    _lightShaderBuffer[to_const_uint(ShaderBufferType::SHADOW)] = _context.newSB(params);

    ResourceDescriptor lightImpostorShader("lightImpostorShader");
    lightImpostorShader.setThreadedLoading(false);
    _lightImpostorShader = CreateResource<ShaderProgram>(_parentScene.resourceCache(), lightImpostorShader);

    SamplerDescriptor iconSampler;
    iconSampler.toggleMipMaps(false);
    iconSampler.setAnisotropy(0);
    iconSampler.setWrapMode(TextureWrap::REPEAT);
    ResourceDescriptor iconImage("LightIconTexture");
    iconImage.setThreadedLoading(false);
    iconImage.setPropertyDescriptor<SamplerDescriptor>(iconSampler);
    stringImpl iconImageLocation =
        Util::StringFormat("%s/misc_images/lightIcons.png",
            ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation")).c_str());
    iconImage.setResourceLocation(iconImageLocation);
    iconImage.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
    _lightIconsTexture = CreateResource<Texture>(_parentScene.resourceCache(), iconImage);

    _init = true;
}

bool LightPool::clear() {
    if (!_init) {
        return true;
    }

    SceneGraph& sceneGraph = _parentScene.sceneGraph();
    for (Light::LightList& lightList : _lights) {
        // Lights are removed by the sceneGraph
        // (range_based for-loops will fail due to iterator invalidation
        vectorAlg::vecSize lightCount = lightList.size();
        for (vectorAlg::vecSize i = lightCount; i--> 0;) {
            Light* crtLight = lightList[i];
            sceneGraph.getRoot().removeNode(*crtLight->getSGN());

        }
        lightList.clear();
    }
    _context.remove2DRenderFunction(_previewShadowMapsCBK);
    _init = false;
    
    return true;
}

bool LightPool::addLight(Light& light) {
    light.addShadowMapInfo(MemoryManager_NEW ShadowMapInfo(&light));

    const LightType type = light.getLightType();
    const U32 lightTypeIdx = to_const_uint(type);

    if (findLight(light.getGUID(), type) != std::end(_lights[lightTypeIdx])) {

        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_DUPLICATE")),
                         light.getGUID());
        return false;
    }

    vectorAlg::emplace_back(_lights[lightTypeIdx], &light);

    return true;
}

// try to remove any leftover lights
bool LightPool::removeLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);

    if (it == std::end(_lights[to_uint(type)])) {
        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_REMOVE_LIGHT")),
                         lightGUID);
        return false;
    }

    _lights[to_uint(type)].erase(it);  // remove it from the map
    return true;
}

void LightPool::idle() {
    _shadowMapsEnabled = ParamHandler::instance().getParam<bool>(_ID("rendering.enableShadows"));
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightPool::generateShadowMaps(GFXDevice& context, SceneRenderState& sceneRenderState) {
    if (!_shadowMapsEnabled) {
        return true;
    }

    waitForTasks();
    ShadowMap::clearShadowMapBuffers(_context);
    Time::ScopedTimer timer(_shadowPassTimer);
    // generate shadowmaps for each light
    I32 idx = 0;
    vectorAlg::vecSize count = _sortedShadowCastingLights.size();
    for (vectorAlg::vecSize i = 0; i < count; ++i) {
        Light* light = _sortedShadowCastingLights[i];
        if(light != nullptr) {
            _currentShadowCastingLight = light;
            light->validateOrCreateShadowMaps(context, sceneRenderState);
            light->generateShadowMaps(context, idx++ * 6);
        }
    }

    _currentShadowCastingLight = nullptr;
    return true;
}

void LightPool::togglePreviewShadowMaps(GFXDevice& context) {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled || context.getRenderStage() != RenderStage::DISPLAY) {
        ParamHandler::instance().setParam( _ID("rendering.debug.displayShadowDebugInfo"), false);
    } else {
        ParamHandler::instance().setParam( _ID("rendering.debug.displayShadowDebugInfo"), _previewShadowMaps);
    }
}

void LightPool::previewShadowMaps(Light* light) {
    if (!Config::Build::IS_DEBUG_BUILD) {
        return;
    }

    // Stop if we have shadows disabled
    if (!_shadowMapsEnabled || !_previewShadowMaps || _lights.empty() ||
        _context.getRenderStage() != RenderStage::DISPLAY) {
        return;
    }

    waitForTasks();
    // If no light is specified show as many shadowmaps as possible
    if (!light) {
        U32 rowIndex = 0;
        for (Light* shadowLight : _sortedShadowCastingLights) {
            if (shadowLight != nullptr &&
                shadowLight->getShadowMapInfo()->getShadowMap() != nullptr)
            {
                shadowLight->getShadowMapInfo()->getShadowMap()->previewShadowMaps(_context, rowIndex++);
            }
        }
        return;
    }

    if (light && light->castsShadows()) {
        assert(light->getShadowMapInfo()->getShadowMap() != nullptr);
        light->getShadowMapInfo()->getShadowMap()->previewShadowMaps(_context, 0);
    }
}

// If we have computed shadowmaps, bind them before rendering any geometry;
void LightPool::bindShadowMaps(GFXDevice& context) {
    // Skip applying shadows if we are rendering to depth map, or we have
    // shadows disabled
    if (!_shadowMapsEnabled) {
        return;
    }

    ShadowMap::bindShadowMaps(context);
}

bool LightPool::shadowMappingEnabled() {
    return _shadowMapsEnabled;
}

Light* LightPool::getLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);
    assert(it != std::end(_lights[to_uint(type)]));

    return *it;
}

void LightPool::waitForTasks() {
    for (TaskHandle& task : _lightUpdateTask) {
        task.wait();
    }
    _lightUpdateTask.clear();
}

void LightPool::prepareLightData(const vec3<F32>& eyePos, const mat4<F32>& viewMatrix) {
    waitForTasks();

    // Create and upload light data for current pass
    _activeLightCount.fill(0);
    _sortedLights.resize(0);
    _sortedShadowCastingLights.resize(0);
    _sortedLightProperties.resize(0);
    _sortedShadowProperties.resize(0);

    for (U8 i = 0; i < to_const_uint(LightType::COUNT); ++i) {
        _sortedLights.insert(std::end(_sortedLights), std::begin(_lights[i]), std::end(_lights[i]));
    }

    auto lightUpdate = [this, &eyePos, &viewMatrix](const Task& parentTask) mutable
    {
        std::sort(std::begin(_sortedLights),
                    std::end(_sortedLights),
                    [&eyePos](Light* a, Light* b) -> bool
        {
            return a->getPosition().distanceSquared(eyePos) <
                    b->getPosition().distanceSquared(eyePos);
        });

        U32 totalLightCount = 0;
        vec3<F32> tempColour;
        for (Light* light : _sortedLights) {
            LightType type = static_cast<LightType>(light->getLightType());
            I32 typeIndex = to_int(type);

            if (!light->getEnabled() || !_lightTypeState[typeIndex]) {
                continue;
            }
            if (totalLightCount >= Config::Lighting::MAX_POSSIBLE_LIGHTS) {
                break;
            }

            _sortedLightProperties.emplace_back();
            LightProperties& temp = _sortedLightProperties.back();
            light->getDiffuseColour(tempColour);
            temp._diffuse.set(tempColour, light->getSpotCosOuterConeAngle());
            // Non directional lights are positioned at specific point in space
            // So we need W = 1 for a valid positional transform
            // Directional lights use position for the light direction. 
            // So we need W = 0 for an infinite distance.
            temp._position.set(viewMatrix.transform(light->getPosition(), type != LightType::DIRECTIONAL), light->getRange());
            // spot direction is not considered a point in space, so W = 0
            temp._direction.set(viewMatrix.transformNonHomogeneous(light->getSpotDirection()), light->getSpotAngle());

            temp._options.x = typeIndex;
            temp._options.y = light->castsShadows();
            if (light->getLightType() == LightType::DIRECTIONAL) {
                temp._options.w = static_cast<DirectionalLight*>(light)->csmSplitCount();
            }
            
           if (light->castsShadows() && _sortedShadowProperties.size() < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
                temp._options.z = to_int(_sortedShadowProperties.size());
                _sortedShadowProperties.emplace_back(light->getShadowProperties());
                _sortedShadowCastingLights.emplace_back(light);
            }
            ++totalLightCount;
            ++_activeLightCount[typeIndex];
        }
        _buffersUpdated = false;
    };

    // Sort all lights (Sort in parallel by type)
    _lightUpdateTask.emplace_back(CreateTask(Application::instance().kernel().taskPool(), lightUpdate));
    _lightUpdateTask.back().startTask(Task::TaskPriority::LOW, 0);
}

void LightPool::uploadLightData(ShaderBufferLocation lightDataLocation,
                                ShaderBufferLocation shadowDataLocation) {
    waitForTasks();
    uploadLightBuffers();

    _lightShaderBuffer[to_const_uint(ShaderBufferType::NORMAL)]->bind(lightDataLocation);
    _lightShaderBuffer[to_const_uint(ShaderBufferType::SHADOW)]->bind(shadowDataLocation);
}

void LightPool::uploadLightBuffers() {
    if (!_buffersUpdated) {
        // Passing 0 elements is fine (early out in the buffer code)
        vectorAlg::vecSize lightPropertyCount = _sortedLightProperties.size();
        vectorAlg::vecSize lightShadowCount = _sortedShadowProperties.size();

        lightPropertyCount = std::min(lightPropertyCount, static_cast<size_t>(Config::Lighting::MAX_POSSIBLE_LIGHTS));
        lightShadowCount = std::min(lightShadowCount, static_cast<size_t>(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS));

        if (lightPropertyCount > 0) {
            _lightShaderBuffer[to_const_uint(ShaderBufferType::NORMAL)]->updateData(0, lightPropertyCount, _sortedLightProperties.data());
        } else {
            _lightShaderBuffer[to_const_uint(ShaderBufferType::NORMAL)]->setData(nullptr);
        }

        if (lightShadowCount > 0) {
            _lightShaderBuffer[to_const_uint(ShaderBufferType::SHADOW)]->updateData(0, lightShadowCount, _sortedShadowProperties.data());
        } else {
            _lightShaderBuffer[to_const_uint(ShaderBufferType::SHADOW)]->setData(nullptr);
        }
        _buffersUpdated = true;
    }
}

void LightPool::drawLightImpostors(RenderSubPassCmds& subPassesInOut) const {
    if (!_previewShadowMaps) {
        return;
    }

    assert(_lightImpostorShader);
    const U32 directionalLightCount = _activeLightCount[to_const_uint(LightType::DIRECTIONAL)];
    const U32 pointLightCount = _activeLightCount[to_const_uint(LightType::POINT)];
    const U32 spotLightCount = _activeLightCount[to_const_uint(LightType::SPOT)];
    const U32 totalLightCount = directionalLightCount + pointLightCount + spotLightCount;
    if (totalLightCount > 0) {
        GenericDrawCommand pointsCmd;
        pointsCmd.primitiveType(PrimitiveType::API_POINTS);
        pointsCmd.drawCount(to_ushort(totalLightCount));
        pointsCmd.stateHash(_context.getDefaultStateBlock(true));
        pointsCmd.shaderProgram(_lightImpostorShader);

        RenderSubPassCmd newSubPass;
        newSubPass._textures.addTexture(TextureData(_lightIconsTexture->getTextureType(),
                                                    _lightIconsTexture->getHandle(),
                                                    to_const_ubyte(ShaderProgram::TextureUsage::UNIT0)));

        newSubPass._commands.push_back(pointsCmd);
        subPassesInOut.push_back(newSubPass);
    }
}

};
