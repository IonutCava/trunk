#include "stdafx.h"

#include "Headers/LightPool.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
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

std::array<U8, to_base(ShadowType::COUNT)> LightPool::_shadowLocation = { {
    to_U8(ShaderProgram::TextureUsage::SHADOW_SINGLE),
    to_U8(ShaderProgram::TextureUsage::SHADOW_LAYERED),
    to_U8(ShaderProgram::TextureUsage::SHADOW_CUBE)
}};

Light* LightPool::_currentShadowCastingLight = nullptr;

bool LightPool::_previewShadowMaps = false;

LightPool::LightPool(Scene& parentScene, GFXDevice& context)
    : SceneComponent(parentScene),
      _context(context),
      _init(false),
      _lightImpostorShader(nullptr),
      _lightIconsTexture(nullptr),
      _shadowBuffer(nullptr),
     // shadowPassTimer is used to measure the CPU-duration of shadow map generation step
     _shadowPassTimer(Time::ADD_TIMER("Shadow Pass Timer"))
{

    _buffersUpdated.fill(false);
    for (U8 i = 0; i < to_U8(RenderStage::COUNT); ++i) {
        _activeLightCount[i].fill(0);
        _lightShaderBuffer.fill(nullptr);
        _sortedLights[i].reserve(Config::Lighting::MAX_POSSIBLE_LIGHTS);
        _sortedLightProperties[i].reserve(Config::Lighting::MAX_POSSIBLE_LIGHTS);
    }

    _sortedShadowProperties.reserve(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

    _lightTypeState.fill(true);
    ParamHandler::instance().setParam<bool>(_ID("rendering.debug.displayShadowDebugInfo"), false);

    init();
}

LightPool::~LightPool()
{
    for (Light::LightList& lightList : _lights) {
        if (!lightList.empty()) {
            Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_LIGHT_LEAKED")));
        }
    }
}

void LightPool::init() {
    if (_init) {
        return;
    }

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = Config::Lighting::MAX_POSSIBLE_LIGHTS;
    bufferDescriptor._primitiveSizeInBytes = sizeof(LightProperties);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = true;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    // NORMAL holds general info about the currently active lights: position, colour, etc.
    for (U8 i = 0; i < to_U8(RenderStage::COUNT); ++i) {
        _lightShaderBuffer[i] = _context.newSB(bufferDescriptor);
    }

    // SHADOWS holds info about the currently active shadow casting lights:
    // ViewProjection Matrices, View Space Position, etc
    // Should be SSBO (not UBO) to use std430 alignment. Structures should not be padded
    bufferDescriptor._primitiveCount = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
    bufferDescriptor._primitiveSizeInBytes = sizeof(Light::ShadowProperties);
    _shadowBuffer = _context.newSB(bufferDescriptor);

    ResourceDescriptor lightImpostorShader("lightImpostorShader");
    lightImpostorShader.setThreadedLoading(false);
    _lightImpostorShader = CreateResource<ShaderProgram>(_parentScene.resourceCache(), lightImpostorShader);

    SamplerDescriptor iconSampler;
    iconSampler.setMinFilter(TextureFilter::LINEAR);
    iconSampler.setAnisotropy(0);
    iconSampler.setWrapMode(TextureWrap::REPEAT);
    iconSampler.toggleSRGBColourSpace(true);

    TextureDescriptor iconDescriptor(TextureType::TEXTURE_2D);
    iconDescriptor.setSampler(iconSampler);

    ResourceDescriptor iconImage("LightIconTexture");
    iconImage.setThreadedLoading(false);
    iconImage.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    iconImage.setResourceName("lightIcons.png");
    iconImage.setPropertyDescriptor(iconDescriptor);

    _lightIconsTexture = CreateResource<Texture>(_parentScene.resourceCache(), iconImage);

    _init = true;
}

bool LightPool::clear() {
    if (!_init) {
        return true;
    }

    if (_parentScene.sceneGraph().removeNodesByType(SceneNodeType::TYPE_LIGHT)) {
        for (Light::LightList& lightList : _lights) {
            lightList.clear();
        }

        return true;
    }

    return false;
}

bool LightPool::addLight(Light& light) {
    light.addShadowMapInfo(MemoryManager_NEW ShadowMapInfo(&light));

    const LightType type = light.getLightType();
    const U32 lightTypeIdx = to_base(type);

    if (findLight(light.getGUID(), type) != std::end(_lights[lightTypeIdx])) {

        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_DUPLICATE")),
                         light.getGUID());
        return false;
    }

    _lights[lightTypeIdx].emplace_back(&light);

    return true;
}

// try to remove any leftover lights
bool LightPool::removeLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);

    if (it == std::end(_lights[to_U32(type)])) {
        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_REMOVE_LIGHT")),
                         lightGUID);
        return false;
    }

    _lights[to_U32(type)].erase(it);  // remove it from the map
    return true;
}

void LightPool::idle() {
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightPool::generateShadowMaps(SceneRenderState& sceneRenderState, const Camera& playerCamera, GFX::CommandBuffer& bufferInOut) {
    if (_context.shadowDetailLevel() == RenderDetailLevel::OFF) {
        return true;
    }

    Time::ScopedTimer timer(_shadowPassTimer);

    ShadowMap::clearShadowMapBuffers(_context);

    _sortedShadowProperties.resize(0);

    LightVec sortedLights;
    for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
        sortedLights.insert(std::end(sortedLights), std::begin(_lights[i]), std::end(_lights[i]));
    }

    const vec3<F32>& eyePos = playerCamera.getEye();

    std::sort(std::begin(sortedLights),
              std::end(sortedLights),
              [&eyePos](Light* a, Light* b) -> bool
              {
                  return a->getPosition().distanceSquared(eyePos) <
                         b->getPosition().distanceSquared(eyePos);
              });


    U32 shadowLightCount = 0;
    for (Light* light : sortedLights) {
        LightType type = static_cast<LightType>(light->getLightType());
        I32 typeIndex = to_I32(type);

        if (!light->getEnabled() || !_lightTypeState[typeIndex]) {
            continue;
        }
        if (light->castsShadows() && shadowLightCount < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
            _sortedShadowProperties.emplace_back(light->getShadowProperties());

            _currentShadowCastingLight = light;
            light->validateOrCreateShadowMaps(_context, sceneRenderState);
            light->generateShadowMaps(shadowLightCount++ * 6, bufferInOut);
        }
    }
    _currentShadowCastingLight = nullptr;

    vec_size lightShadowCount = _sortedShadowProperties.size();
    lightShadowCount = std::min(lightShadowCount, static_cast<size_t>(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS));
    if (lightShadowCount > 0) {
        _shadowBuffer->writeData(0, lightShadowCount, _sortedShadowProperties.data());
    } else {
        _shadowBuffer->writeData(nullptr);
    }

    return true;
}

void LightPool::togglePreviewShadowMaps(GFXDevice& context) {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (context.shadowDetailLevel() == RenderDetailLevel::OFF) {
        _previewShadowMaps = false;
    }

    ParamHandler::instance().setParam(_ID("rendering.debug.displayShadowDebugInfo"), _previewShadowMaps);
}

// If we have computed shadowmaps, bind them before rendering any geometry;
void LightPool::bindShadowMaps(GFXDevice& context, GFX::CommandBuffer& bufferInOut) {
    // Skip applying shadows if we are rendering to depth map, or we have
    // shadows disabled
    if (context.shadowDetailLevel() == RenderDetailLevel::OFF) {
        return;
    }

    ShadowMap::bindShadowMaps(context, bufferInOut);
}

Light* LightPool::getLight(I64 lightGUID, LightType type) {
    Light::LightList::const_iterator it = findLight(lightGUID, type);
    assert(it != std::end(_lights[to_U32(type)]));

    return *it;
}

void LightPool::waitForTasks(RenderStagePass stagePass) {
    _lightUpdateTask[to_base(stagePass._stage)].wait();
}

void LightPool::prepareLightData(RenderStagePass stagePass, const vec3<F32>& eyePos, const mat4<F32>& viewMatrix) {
    waitForTasks(stagePass);

    U8 stageIndex = to_U8(stagePass._stage);

    // Create and upload light data for current pass
    _activeLightCount[stageIndex].fill(0);
    _sortedLights[stageIndex].resize(0);
    _sortedLightProperties[stageIndex].resize(0);

    for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
        _sortedLights[stageIndex].insert(std::end(_sortedLights[stageIndex]), std::begin(_lights[i]), std::end(_lights[i]));
    }

    auto lightUpdate = [this, stageIndex, &eyePos, &viewMatrix](const Task& parentTask) mutable
    {
        std::sort(std::begin(_sortedLights[stageIndex]),
                  std::end(_sortedLights[stageIndex]),
                  [&eyePos](Light* a, Light* b) -> bool
                  {
                       return a->getPosition().distanceSquared(eyePos) <
                              b->getPosition().distanceSquared(eyePos);
                  });

        U32 totalLightCount = 0;
        vec3<F32> tempColour;
        for (Light* light : _sortedLights[stageIndex]) {
            LightType type = static_cast<LightType>(light->getLightType());
            I32 typeIndex = to_I32(type);

            if (!light->getEnabled() || !_lightTypeState[typeIndex]) {
                continue;
            }
            if (totalLightCount >= Config::Lighting::MAX_POSSIBLE_LIGHTS) {
                break;
            }

            _sortedLightProperties[stageIndex].emplace_back();
            LightProperties& temp = _sortedLightProperties[stageIndex].back();
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

            ++totalLightCount;
            ++_activeLightCount[stageIndex][typeIndex];
        }
        _buffersUpdated[stageIndex] = false;
    };

    // Sort all lights (Sort in parallel by type)
    _lightUpdateTask[stageIndex] = CreateTask(_context.context().taskPool(), lightUpdate);
    _lightUpdateTask[stageIndex].startTask();
}

void LightPool::uploadLightData(RenderStagePass stagePass,
                                ShaderBufferLocation lightDataLocation,
                                ShaderBufferLocation shadowDataLocation,
                                GFX::CommandBuffer& bufferInOut) {

    GFX::ExternalCommand externalCmd;
    externalCmd._cbk = [this, stagePass]() {
        waitForTasks(stagePass);
        uploadLightBuffers(stagePass);
    };
    GFX::EnqueueCommand(bufferInOut, externalCmd);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = _context.newDescriptorSet();
    descriptorSetCmd._set->_shaderBuffers.emplace_back(lightDataLocation, _lightShaderBuffer[to_base(stagePass._stage)]);
    descriptorSetCmd._set->_shaderBuffers.emplace_back(shadowDataLocation, _shadowBuffer);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void LightPool::uploadLightBuffers(RenderStagePass stagePass) {
    U8 stageIndex = to_U8(stagePass._stage);

    if (!_buffersUpdated[stageIndex]) {
        // Passing 0 elements is fine (early out in the buffer code)
        vec_size lightPropertyCount = _sortedLightProperties.size();

        lightPropertyCount = std::min(lightPropertyCount, static_cast<size_t>(Config::Lighting::MAX_POSSIBLE_LIGHTS));

        if (lightPropertyCount > 0) {
            _lightShaderBuffer[stageIndex]->writeData(0, lightPropertyCount, _sortedLightProperties.data());
        } else {
            _lightShaderBuffer[stageIndex]->writeData(nullptr);
        }

        _buffersUpdated[stageIndex] = true;
    }
}

void LightPool::drawLightImpostors(RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) const {
    if (!_previewShadowMaps) {
        return;
    }

    U8 stageIndex = to_U8(stagePass._stage);

    assert(_lightImpostorShader);
    const U32 directionalLightCount = _activeLightCount[stageIndex][to_base(LightType::DIRECTIONAL)];
    const U32 pointLightCount = _activeLightCount[stageIndex][to_base(LightType::POINT)];
    const U32 spotLightCount = _activeLightCount[stageIndex][to_base(LightType::SPOT)];
    const U32 totalLightCount = directionalLightCount + pointLightCount + spotLightCount;
    if (totalLightCount > 0) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.getDefaultStateBlock(true);
        pipelineDescriptor._shaderProgramHandle = _lightImpostorShader->getID();

        GenericDrawCommand pointsCmd;
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = to_U16(totalLightCount);

        GFX::BindPipelineCommand bindPipeline;
        bindPipeline._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, bindPipeline);
        
        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(TextureData(_lightIconsTexture->getTextureType(), _lightIconsTexture->getHandle()),
                                                       to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(pointsCmd);
        GFX::EnqueueCommand(bufferInOut, drawCommand);
    }
}

};
