#include "stdafx.h"

#include "Headers/LightPool.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
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
    ReadLock r_lock(_lightLock);
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
    bufferDescriptor._primitiveCount = 1;
    bufferDescriptor._primitiveSizeInBytes = sizeof(vec4<I32>) + (Config::Lighting::MAX_POSSIBLE_LIGHTS * sizeof(LightProperties));
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) | to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    // NORMAL holds general info about the currently active lights: position, colour, etc.
    for (U8 i = 0; i < to_U8(RenderStage::COUNT); ++i) {
        bufferDescriptor._name = Util::StringFormat("LIGHT_BUFFER_%s", 
                                                    TypeUtil::renderStageToString(static_cast<RenderStage>(i))).c_str();
        _lightShaderBuffer[i] = _context.newSB(bufferDescriptor);
    }

    // SHADOWS holds info about the currently active shadow casting lights:
    // ViewProjection Matrices, View Space Position, etc
    // Should be SSBO (not UBO) to use std430 alignment. Structures should not be padded
    bufferDescriptor._primitiveCount = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
    bufferDescriptor._primitiveSizeInBytes = sizeof(Light::ShadowProperties);
    bufferDescriptor._name = "LIGHT_SHADOW_BUFFER";
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
        WriteLock w_lock(_lightLock);

        for (Light::LightList& lightList : _lights) {
            lightList.clear();
        }

        return true;
    }

    return false;
}

bool LightPool::addLight(Light& light) {
    const LightType type = light.getLightType();
    const U32 lightTypeIdx = to_base(type);

    UpgradableReadLock ur_lock(_lightLock);
    if (findLightLocked(light.getGUID(), type) != std::end(_lights[lightTypeIdx])) {

        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_DUPLICATE")),
                         light.getGUID());
        return false;
    }

    UpgradeToWriteLock w_lock(ur_lock);
    _lights[lightTypeIdx].emplace_back(&light);
    light.initDebugViews(_context);

    return true;
}

// try to remove any leftover lights
bool LightPool::removeLight(I64 lightGUID, LightType type) {
    UpgradableReadLock ur_lock(_lightLock);
    Light::LightList::const_iterator it = findLightLocked(lightGUID, type);

    if (it == std::end(_lights[to_U32(type)])) {
        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_REMOVE_LIGHT")),
                         lightGUID);
        return false;
    }

    UpgradeToWriteLock w_lock(ur_lock);
    _lights[to_U32(type)].erase(it);  // remove it from the map
    return true;
}

void LightPool::idle() {
}

/// When pre-rendering is done, the Light Manager will generate the shadow maps
/// Returning false in any of the FrameListener methods will exit the entire
/// application!
bool LightPool::generateShadowMaps(const Camera& playerCamera, GFX::CommandBuffer& bufferInOut) {
    Time::ScopedTimer timer(_shadowPassTimer);

    ShadowMap::clearShadowMapBuffers(bufferInOut);
    ShadowMap::resetShadowMaps();

    LightVec sortedLights;
    U32 shadowLightCount = 0;
    shadowCastingLights(playerCamera.getEye(), sortedLights);

    _sortedShadowProperties.clear();

    for (Light* light : sortedLights) {
        if (_sortedShadowProperties.size() >= Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
            light->updateDebugViews(false, 0);
        } else {
            U32 offset = shadowLightCount++ * Config::Lighting::MAX_SPLITS_PER_LIGHT;
            ShadowMap::generateShadowMaps(playerCamera, *light, offset, bufferInOut);
             _sortedShadowProperties.emplace_back(light->getShadowProperties());
             light->updateDebugViews(true, offset);
         }
    }

    vec_size lightShadowCount = std::min(_sortedShadowProperties.size(), static_cast<size_t>(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS));
    _shadowBuffer->writeData(0, lightShadowCount, _sortedShadowProperties.data());

    return true;
}

void LightPool::shadowCastingLights(const vec3<F32>& eyePos, LightVec& sortedShadowLights) const {

    sortedShadowLights.resize(0);
    sortedShadowLights.reserve(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

    LightVec sortedLights;
    {
        ReadLock r_lock(_lightLock);
        sortedLights.reserve(_lights.size());

        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            for (Light* light : _lights[i]) {
                sortedLights.push_back(light);
            }
        }
    }
    std::sort(std::begin(sortedLights),
              std::end(sortedLights),
              [&eyePos](Light* a, Light* b) -> bool
              {
                  return a->getLightType() == LightType::DIRECTIONAL ||
                          (a->getPosition().distanceSquared(eyePos) <
                           b->getPosition().distanceSquared(eyePos));
              });

    for (Light* light : sortedLights) {
        if (!light->getEnabled() ||
            !light->castsShadows() ||
            !_lightTypeState[to_base(light->getLightType())])
        {
            continue;
        }

        sortedShadowLights.push_back(light);

        if (sortedShadowLights.size() == Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
            break;
        }
    }

}

void LightPool::togglePreviewShadowMaps(GFXDevice& context) {
    _previewShadowMaps = !_previewShadowMaps;
    // Stop if we have shadows disabled
    if (!context.parent().platformContext().config().rendering.shadowMapping.enabled) {
        _previewShadowMaps = false;
    }

    ParamHandler::instance().setParam(_ID("rendering.debug.displayShadowDebugInfo"), _previewShadowMaps);
}

// If we have computed shadowmaps, bind them before rendering any geometry;
void LightPool::bindShadowMaps(GFXDevice& context, GFX::CommandBuffer& bufferInOut) {
    // Skip applying shadows if we are rendering to depth map, or we have shadows disabled
    if (context.parent().platformContext().config().rendering.shadowMapping.enabled) {
        ShadowMap::bindShadowMaps(context, bufferInOut);
    }
}

Light* LightPool::getLight(I64 lightGUID, LightType type) {
    ReadLock r_lock(_lightLock);

    Light::LightList::const_iterator it = findLight(lightGUID, type);
    assert(it != eastl::end(_lights[to_U32(type)]));

    return *it;
}

void LightPool::waitForTasks(U8 stageIndex) {
    _lightUpdateTask[stageIndex].wait();
}

void LightPool::prepareLightData(RenderStage stage, const vec3<F32>& eyePos, const mat4<F32>& viewMatrix) {
    U8 stageIndex = to_U8(stage);
    waitForTasks(stageIndex);

    // Create and upload light data for current pass
    _activeLightCount[stageIndex].fill(0);
    _sortedLights[stageIndex].resize(0);
    _sortedLightProperties[stageIndex].resize(0);

    {
        ReadLock r_lock(_lightLock);
        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            for (Light* light : _lights[i]) {
                _sortedLights[stageIndex].push_back(light);
            }
        }
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
        uploadLightBuffers(eyePos, stageIndex);
        _buffersUpdated[stageIndex] = false;
    };

    // Sort all lights (Sort in parallel by type)
    _lightUpdateTask[stageIndex] = CreateTask(_context.context().taskPool(), lightUpdate);
    _lightUpdateTask[stageIndex].startTask();
}

void LightPool::uploadLightData(RenderStage stage,
                                ShaderBufferLocation lightDataLocation,
                                ShaderBufferLocation shadowDataLocation,
                                GFX::CommandBuffer& bufferInOut) {

    GFX::ExternalCommand externalCmd;
    externalCmd._cbk = [this, stage]() {
        waitForTasks(to_U8(stage));
    };
    GFX::EnqueueCommand(bufferInOut, externalCmd);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = _context.newDescriptorSet();
    descriptorSetCmd._set->_shaderBuffers.emplace_back(lightDataLocation, _lightShaderBuffer[to_base(stage)]);
    descriptorSetCmd._set->_shaderBuffers.emplace_back(shadowDataLocation, _shadowBuffer);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void LightPool::uploadLightBuffers(const vec3<F32>& eyePos, U8 stageIndex) {

    if (!_buffersUpdated[stageIndex]) {
        // Passing 0 elements is fine (early out in the buffer code)
        vec_size lightPropertyCount = _sortedLightProperties[stageIndex].size();

        LightVec sortedLights;
        shadowCastingLights(eyePos, sortedLights);

        vec4<I32> properties(to_I32(lightPropertyCount),
                             _activeLightCount[stageIndex][to_base(LightType::DIRECTIONAL)],
                             _activeLightCount[stageIndex][to_base(LightType::POINT)],
                             to_I32(sortedLights.size()));
        _lightShaderBuffer[stageIndex]->writeBytes(0, sizeof(vec4<I32>), properties._v);

        lightPropertyCount = std::min(lightPropertyCount, static_cast<size_t>(Config::Lighting::MAX_POSSIBLE_LIGHTS));
        _lightShaderBuffer[stageIndex]->writeBytes(sizeof(vec4<I32>), lightPropertyCount * sizeof(LightProperties), lightPropertyCount > 0 ? _sortedLightProperties[stageIndex].data() : nullptr);

        _buffersUpdated[stageIndex] = true;
    }
}

void LightPool::drawLightImpostors(RenderStage stage, GFX::CommandBuffer& bufferInOut) const {
    if (!_previewShadowMaps) {
        return;
    }

    U8 stageIndex = to_U8(stage);

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
