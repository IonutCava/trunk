#include "stdafx.h"

#include "Headers/LightPool.h"

#include "Core/Headers/Kernel.h"
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

#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

std::array<U8, to_base(ShadowType::COUNT)> LightPool::_shadowLocation = { {
    to_U8(ShaderProgram::TextureUsage::SHADOW_SINGLE),
    to_U8(ShaderProgram::TextureUsage::SHADOW_LAYERED),
    to_U8(ShaderProgram::TextureUsage::SHADOW_CUBE)
}};

namespace {
    LightPool::LightList g_sortedLightsContainer = {};
};

bool LightPool::_debugDraw = false;

LightPool::LightPool(Scene& parentScene, PlatformContext& context)
    : SceneComponent(parentScene),
      PlatformContextComponent(context),
      _init(false),
      _lightImpostorShader(nullptr),
      _lightIconsTexture(nullptr),
      _lightShaderBuffer(nullptr),
      _shadowBuffer(nullptr),
     // shadowPassTimer is used to measure the CPU-duration of shadow map generation step
     _shadowPassTimer(Time::ADD_TIMER("Shadow Pass Timer"))
{

    for (U8 i = 0; i < to_U8(RenderStage::COUNT); ++i) {
        _activeLightCount[i].fill(0);
        _sortedLights[i].reserve(Config::Lighting::MAX_POSSIBLE_LIGHTS);
    }

    _lightTypeState.fill(true);
    ParamHandler::instance().setParam<bool>(_ID_32("rendering.debug.displayShadowDebugInfo"), false);

    init();
}

LightPool::~LightPool()
{
    SharedLock r_lock(_lightLock);
    for (LightList& lightList : _lights) {
        if (!lightList.empty()) {
            Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_LIGHT_LEAKED")));
        }
    }
}

void LightPool::init() {
    if (_init) {
        return;
    }

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = to_base(RenderStage::COUNT) - 1; //< no shadows
    bufferDescriptor._elementSize = sizeof(vec4<U32>) + (Config::Lighting::MAX_POSSIBLE_LIGHTS * sizeof(LightProperties));
    bufferDescriptor._ringBufferLength = 6;
    bufferDescriptor._separateReadWrite = false;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::IMMUTABLE_STORAGE) | 
                              to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES) |
                              to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH);

    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
    bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    bufferDescriptor._name = "LIGHT_BUFFER";
    // NORMAL holds general info about the currently active lights: position, colour, etc.
    _lightShaderBuffer = _context.gfx().newSB(bufferDescriptor);

    // SHADOWS holds info about the currently active shadow casting lights:
    // ViewProjection Matrices, View Space Position, etc
    bufferDescriptor._usage = ShaderBuffer::Usage::CONSTANT_BUFFER;
    bufferDescriptor._elementCount = 1;
    bufferDescriptor._elementSize = sizeof(ShadowProperties);
    bufferDescriptor._name = "LIGHT_SHADOW_BUFFER";
    
    _shadowBuffer = _context.gfx().newSB(bufferDescriptor);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "lightImpostorShader.glsl";

    ShaderModuleDescriptor geomModule = {};
    geomModule._moduleType = ShaderType::GEOMETRY;
    geomModule._sourceFile = "lightImpostorShader.glsl";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "lightImpostorShader.glsl";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(geomModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor lightImpostorShader("lightImpostorShader");
    lightImpostorShader.threaded(false);
    lightImpostorShader.propertyDescriptor(shaderDescriptor);
    lightImpostorShader.waitForReady(false);
    _lightImpostorShader = CreateResource<ShaderProgram>(_parentScene.resourceCache(), lightImpostorShader);

    SamplerDescriptor iconSampler = {};
    iconSampler.wrapU(TextureWrap::REPEAT);
    iconSampler.wrapV(TextureWrap::REPEAT);
    iconSampler.wrapW(TextureWrap::REPEAT);
    iconSampler.minFilter(TextureFilter::LINEAR);
    iconSampler.magFilter(TextureFilter::LINEAR);
    iconSampler.anisotropyLevel(0);

    TextureDescriptor iconDescriptor(TextureType::TEXTURE_2D);
    iconDescriptor.samplerDescriptor(iconSampler);
    iconDescriptor.srgb(true);

    ResourceDescriptor iconImage("LightIconTexture");
    iconImage.threaded(false);
    iconImage.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    iconImage.assetName("lightIcons.png");
    iconImage.propertyDescriptor(iconDescriptor);

    _lightIconsTexture = CreateResource<Texture>(_parentScene.resourceCache(), iconImage);

    _init = true;
}

bool LightPool::clear() {
    if (!_init) {
        return true;
    }
    g_sortedLightsContainer.clear();

    return _lights.empty();
}

bool LightPool::addLight(Light& light) {
    const LightType type = light.getLightType();
    const U32 lightTypeIdx = to_base(type);

    UniqueLockShared r_lock(_lightLock);
    if (findLightLocked(light.getGUID(), type) != eastl::end(_lights[lightTypeIdx])) {

        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_DUPLICATE")),
                         light.getGUID());
        return false;
    }

    _lights[lightTypeIdx].emplace_back(&light);

    return true;
}

// try to remove any leftover lights
bool LightPool::removeLight(Light& light) {
    UniqueLockShared lock(_lightLock);
    LightList::const_iterator it = findLightLocked(light.getGUID(), light.getLightType());

    if (it == eastl::end(_lights[to_U32(light.getLightType())])) {
        Console::errorfn(Locale::get(_ID("ERROR_LIGHT_POOL_REMOVE_LIGHT")),
                         light.getGUID());
        return false;
    }

    _lights[to_U32(light.getLightType())].erase(it);  // remove it from the map
    return true;
}

void LightPool::idle() {
}

void LightPool::generateShadowMaps(const Camera& playerCamera, GFX::CommandBuffer& bufferInOut) {
    Time::ScopedTimer timer(_shadowPassTimer);

    ShadowMap::clearShadowMapBuffers(bufferInOut);
    ShadowMap::resetShadowMaps();

    for (vec4<F32>& details : _shadowBufferData._lightDetails) {
        details.x = to_F32(LightType::COUNT);
    }

    if (!_sortedShadowLights.empty()) {

        U32 shadowLightCount = 0;
        U32 directionalLightCount = 0;
        for (Light* light : _sortedShadowLights) {
            if (shadowLightCount >= Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
                break;
            }

            const bool isDirLight = light->getLightType() == LightType::DIRECTIONAL;

            if (!isDirLight || directionalLightCount < Config::Lighting::MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS) {
                ShadowMap::generateShadowMaps(playerCamera, *light, shadowLightCount, bufferInOut);

                const Light::ShadowProperties& shadowProp = light->getShadowProperties();

                _shadowBufferData._lightDetails[shadowLightCount].set(shadowProp._lightDetails);

                std::memcpy(&_shadowBufferData._lightPosition[shadowLightCount * 6]._v, &shadowProp._lightPosition[0]._v, ShadowMap::MAX_PASSES_PER_LIGHT * sizeof(vec4<F32>));
                std::memcpy(&_shadowBufferData._lightVP[shadowLightCount * 6].mat,      &shadowProp._lightVP[0].mat,      ShadowMap::MAX_PASSES_PER_LIGHT * sizeof(mat4<F32>));

                ++shadowLightCount;
                if (isDirLight) {
                    ++directionalLightCount;
                }
            }
        }

        _shadowBuffer->writeData(_shadowBufferData.data());
    }

    ShaderBufferBinding buffer = {};
    buffer._binding = ShaderBufferLocation::LIGHT_SHADOW;
    buffer._buffer = _shadowBuffer;
    buffer._elementRange = { 0u, _shadowBuffer->getPrimitiveCount() };

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set.addShaderBuffer(buffer);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    ShadowMap::bindShadowMaps(bufferInOut);
}

void LightPool::togglePreviewShadowMaps(GFXDevice& context, Light& light) {
    _debugDraw = !_debugDraw;
    // Stop if we have shadows disabled
    if (!context.context().config().rendering.shadowMapping.enabled) {
        return;
    }

    ParamHandler::instance().setParam(_ID_32("rendering.debug.displayShadowDebugInfo"), _debugDraw);
    if (_debugDraw) {
        ShadowMap::enableShadowDebugViewForLight(context, light);
    } else {
        ShadowMap::disableShadowDebugViews(context);
    }
}

Light* LightPool::getLight(I64 lightGUID, LightType type) {
    SharedLock r_lock(_lightLock);

    LightList::const_iterator it = findLight(lightGUID, type);
    assert(it != eastl::end(_lights[to_U32(type)]));

    return *it;
}

// This should be called in a separate thread for each RenderStage
void LightPool::prepareLightData(RenderStage stage, const vec3<F32>& eyePos, const mat4<F32>& viewMatrix) {
    U8 stageIndex = to_U8(stage);

    LightList& sortedLights = _sortedLights[stageIndex];
    sortedLights.resize(0);
    {
        SharedLock r_lock(_lightLock);
        size_t totalLightCount = 0;
        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            totalLightCount += _lights[i].size();
        }
        sortedLights.reserve(totalLightCount);

        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            sortedLights.insert(eastl::cend(sortedLights), eastl::cbegin(_lights[i]), eastl::cend(_lights[i]));
        }
    }
    {
        OPTICK_EVENT("LightPool::SortLights");

        eastl::sort(eastl::begin(sortedLights),
                    eastl::end(sortedLights),
                    [&eyePos](Light* a, Light* b) {
                        // directional lights first
                        if (a->getLightType() != b->getLightType()) {
                            return to_base(a->getLightType()) < to_base(b->getLightType());
                        }
                        return a->distanceSquared(eyePos) < b->distanceSquared(eyePos);
                    });
    }
    U32 totalLightCount = 0;
    vec3<F32> tempColour;

    auto& lightCount = _activeLightCount[stageIndex];
    lightCount.fill(0);
        
    BufferData& crtData = _sortedLightProperties[stageIndex];
    for (Light* light : sortedLights) {
        const LightType type = static_cast<LightType>(light->getLightType());
        const bool isDirectional = type == LightType::DIRECTIONAL;
        const bool isOmni = type == LightType::POINT;

        const I32 typeIndex = to_I32(type);

        if (_lightTypeState[typeIndex] && light->enabled()) {
            if (totalLightCount++ >= Config::Lighting::MAX_POSSIBLE_LIGHTS) {
                break;
            }

            LightProperties& temp = crtData._lightProperties[totalLightCount - 1];
            light->getDiffuseColour(tempColour);
            temp._diffuse.set(tempColour, light->getSpotCosOuterConeAngle());
            // Omni and spot lights have a position. Directional lights have this set to (0,0,0)
            if (isDirectional) {
                temp._position.set(VECTOR3_ZERO, light->getRange());
            } else {
                temp._position.set((viewMatrix * vec4<F32>(light->positionCache(), 1.0f)).xyz(), light->getRange());
            }

            if (isOmni) {
                temp._direction.set(VECTOR3_ZERO, light->getConeAngle());
            } else {
                temp._direction.set((viewMatrix * vec4<F32>(light->directionCache(), 0.0f)).xyz(), light->getConeAngle());
            }
            temp._options.xy(typeIndex, light->shadowIndex());

            ++lightCount[typeIndex];
        }
    }
        
    crtData._globalData.set(
        _activeLightCount[stageIndex][to_base(LightType::DIRECTIONAL)],
        _activeLightCount[stageIndex][to_base(LightType::POINT)],
        _activeLightCount[stageIndex][to_base(LightType::SPOT)],
        to_U32(_sortedShadowLights.size())
    );
    {
        OPTICK_EVENT("LightPool::UploadLightDataToGPU");

        _lightShaderBuffer->writeBytes((stageIndex - 1) * _lightShaderBuffer->getPrimitiveSize(),
                                       sizeof(vec4<I32>) + (totalLightCount * sizeof(LightProperties)),
                                       (bufferPtr)(&crtData));
    }
}

void LightPool::uploadLightData(RenderStage stage, GFX::CommandBuffer& bufferInOut) {

    ShaderBufferBinding buffer = {};
    buffer._binding = ShaderBufferLocation::LIGHT_NORMAL;
    buffer._buffer = _lightShaderBuffer;
    buffer._elementRange = { to_base(stage) - 1, 1 };

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set.addShaderBuffer(buffer);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void LightPool::preRenderAllPasses(const Camera& playerCamera) {
    OPTICK_EVENT();

    const vec3<F32>& eyePos = playerCamera.getEye();

    _sortedShadowLights.resize(0);
    _sortedShadowLights.reserve(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

    g_sortedLightsContainer.resize(0);
    {
        SharedLock r_lock(_lightLock);
        size_t totalLightCount = 0;
        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            totalLightCount += _lights[i].size();
        }
        g_sortedLightsContainer.reserve(totalLightCount);

        for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
            g_sortedLightsContainer.insert(eastl::cend(g_sortedLightsContainer), eastl::cbegin(_lights[i]), eastl::cend(_lights[i]));
        }
    }

    eastl::sort(eastl::begin(g_sortedLightsContainer),
                eastl::end(g_sortedLightsContainer),
                [&eyePos](Light* a, Light* b) {
                    // directional lights first
                    if (a->getLightType() != b->getLightType()) {
                        return to_base(a->getLightType()) < to_base(b->getLightType());
                    }
                    return a->positionCache().distanceSquared(eyePos) < b->positionCache().distanceSquared(eyePos);
                });

    for (Light* light : g_sortedLightsContainer) {
        light->shadowIndex(-1);
        if (light->enabled() && light->castsShadows() && _lightTypeState[to_base(light->getLightType())]) {
            if (_sortedShadowLights.size() < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS) {
                light->shadowIndex(to_I32(_sortedShadowLights.size()));
                _sortedShadowLights.push_back(light);
            }
        }
    }
}

void LightPool::postRenderAllPasses(const Camera& playerCamera) {
    ACKNOWLEDGE_UNUSED(playerCamera);

    // Move backwards so that we don't step on our own toes with the lockmanager
    _lightShaderBuffer->decQueue();
    _shadowBuffer->decQueue();
}

void LightPool::drawLightImpostors(RenderStage stage, GFX::CommandBuffer& bufferInOut) const {
    if (!_debugDraw) {
        return;
    }

    U8 stageIndex = to_U8(stage);

    assert(_lightImpostorShader);

    U32 totalLightCount = 0;
    for (U8 i = 0; i < to_base(LightType::COUNT); ++i) {
        totalLightCount += _activeLightCount[stageIndex][i];
    }

    if (totalLightCount > 0) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.gfx().getDefaultStateBlock(false);
        pipelineDescriptor._shaderProgramHandle = _lightImpostorShader->getGUID();

        GenericDrawCommand pointsCmd;
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = to_U16(totalLightCount);

        GFX::BindPipelineCommand bindPipeline;
        bindPipeline._pipeline = _context.gfx().newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, bindPipeline);
        
        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.setTexture(_lightIconsTexture->data(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::DrawCommand drawCommand = { pointsCmd };
        GFX::EnqueueCommand(bufferInOut, drawCommand);
    }
}

};
