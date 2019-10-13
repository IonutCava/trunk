#include "stdafx.h"

#include "Headers/PostFX.h"
#include "Headers/PreRenderOperator.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

PostFX::PostFX(PlatformContext& context, ResourceCache& cache)
    : PlatformContextComponent(context),
      _preRenderBatch(nullptr),
      _screenBorder(nullptr),
      _noise(nullptr),
      _randomNoiseCoefficient(0.0f),
      _randomFlashCoefficient(0.0f),
      _noiseTimer(0.0),
      _tickInterval(1),
      _postProcessingShader(nullptr),
      _underwaterTexture(nullptr),
      _drawPipeline(nullptr),
      _filtersDirty(true),
      _filterStack(0),
      _currentFadeTimeMS(0.0),
      _targetFadeTimeMS(0.0),
      _fadeWaitDurationMS(0.0),
      _fadeOut(false),
      _fadeActive(false)
{
    ParamHandler::instance().setParam<bool>(_ID("postProcessing.enableVignette"), false);

    _postFXTarget.drawMask().disableAll();
    _postFXTarget.drawMask().setEnabled(RTAttachmentType::Colour, 0, true);

    Console::printfn(Locale::get(_ID("START_POST_FX")));
    _preRenderBatch = MemoryManager_NEW PreRenderBatch(context.gfx(), cache);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "postProcessing.glsl";
    fragModule._defines.push_back(std::make_pair(Util::StringFormat("TEX_BIND_POINT_SCREEN %d", to_base(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN)), true));
    fragModule._defines.push_back(std::make_pair(Util::StringFormat("TEX_BIND_POINT_NOISE %d", to_base(TexOperatorBindPoint::TEX_BIND_POINT_NOISE)), true));
    fragModule._defines.push_back(std::make_pair(Util::StringFormat("TEX_BIND_POINT_BORDER %d", to_base(TexOperatorBindPoint::TEX_BIND_POINT_BORDER)), true));
    fragModule._defines.push_back(std::make_pair(Util::StringFormat("TEX_BIND_POINT_UNDERWATER %d", to_base(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER)), true));

    ShaderProgramDescriptor postFXShaderDescriptor = {};
    postFXShaderDescriptor._modules.push_back(vertModule);
    postFXShaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor postFXShader("postProcessing");
    postFXShader.threaded(false);
    postFXShader.propertyDescriptor(postFXShaderDescriptor);
    _postProcessingShader = CreateResource<ShaderProgram>(cache, postFXShader);
    _drawConstants.set("_noiseTile", GFX::PushConstantType::FLOAT, 0.1f);
    _drawConstants.set("_noiseFactor", GFX::PushConstantType::FLOAT, 0.02f);
    _drawConstants.set("_fadeActive", GFX::PushConstantType::BOOL, false);
    _drawConstants.set("_zPlanes", GFX::PushConstantType::VEC2, vec2<F32>(0.01f, 500.0f));

    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(ShaderType::FRAGMENT, "Vignette"));  // 0
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(ShaderType::FRAGMENT, "Noise"));  // 1
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(ShaderType::FRAGMENT, "screenUnderwater"));  // 2
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(ShaderType::FRAGMENT, "screenNormal"));  // 3
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(ShaderType::FRAGMENT, "ColourPassThrough"));  // 4

    _shaderFunctionSelection.resize(_postProcessingShader->GetSubroutineUniformCount(ShaderType::FRAGMENT), 0);

    SamplerDescriptor defaultSampler = {};
    defaultSampler.wrapU(TextureWrap::REPEAT);
    defaultSampler.wrapV(TextureWrap::REPEAT);
    defaultSampler.wrapW(TextureWrap::REPEAT);

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D);
    texDescriptor.samplerDescriptor(defaultSampler);

    ResourceDescriptor textureWaterCaustics("Underwater Caustics");
    textureWaterCaustics.assetName("terrain_water_NM.jpg");
    textureWaterCaustics.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    textureWaterCaustics.propertyDescriptor(texDescriptor);
    textureWaterCaustics.threaded(false);
    _underwaterTexture = CreateResource<Texture>(cache, textureWaterCaustics);

    ResourceDescriptor noiseTexture("noiseTexture");
    noiseTexture.assetName("bruit_gaussien.jpg");
    noiseTexture.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    noiseTexture.propertyDescriptor(texDescriptor);
    noiseTexture.threaded(false);
    _noise = CreateResource<Texture>(cache, noiseTexture);

    ResourceDescriptor borderTexture("borderTexture");
    borderTexture.assetName("vignette.jpeg");
    borderTexture.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    borderTexture.propertyDescriptor(texDescriptor);
    borderTexture.threaded(false);
    _screenBorder = CreateResource<Texture>(cache, borderTexture);

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = context.gfx().get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _postProcessingShader->getGUID();

    _drawCommand._primitiveType = PrimitiveType::TRIANGLES;
    _drawCommand._drawCount = 1;
    _drawPipeline = context.gfx().newPipeline(pipelineDescriptor);

    _preRenderBatch->init(RenderTargetID(RenderTargetUsage::SCREEN));

    _noiseTimer = 0.0;
    _tickInterval = 1.0f / 24.0f;
    _randomNoiseCoefficient = 0;
    _randomFlashCoefficient = 0;
}

PostFX::~PostFX()
{
    if (_preRenderBatch) {
        _preRenderBatch->destroy();
        MemoryManager::SAFE_DELETE(_preRenderBatch);
    }
}

void PostFX::updateResolution(U16 width, U16 height) {
    if ((_resolutionCache.width == width &&
         _resolutionCache.height == height)|| 
        width < 1 || height < 1)
    {
        return;
    }

    _resolutionCache.set(width, height);

    _preRenderBatch->reshape(width, height);
}

void PostFX::apply(const Camera& camera, GFX::CommandBuffer& bufferInOut) {
    if (_filtersDirty) {
        _shaderFunctionSelection[0] = _shaderFunctionList[getFilterState(FilterType::FILTER_VIGNETTE) ? 0 : 4];
        _shaderFunctionSelection[1] = _shaderFunctionList[getFilterState(FilterType::FILTER_NOISE) ? 1 : 4];
        _shaderFunctionSelection[2] = _shaderFunctionList[getFilterState(FilterType::FILTER_UNDERWATER) ? 2 : 3];

        PipelineDescriptor desc = _drawPipeline->descriptor();
        desc._shaderFunctions[to_base(ShaderType::FRAGMENT)] = _shaderFunctionSelection;
        _drawPipeline = context().gfx().newPipeline(desc);
        _filtersDirty = false;
    }

    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    _preRenderBatch->execute(camera, _filterStack, bufferInOut);

    TextureData output = _preRenderBatch->getOutput();
    TextureData data0 = _underwaterTexture->data();
    TextureData data1 = _noise->data();
    TextureData data2 = _screenBorder->data();

    RenderTarget& screenRT = context().gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    TextureData depthData = screenRT.getAttachment(RTAttachmentType::Depth, 0).texture()->data();

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
    beginRenderPassCmd._descriptor = _postFXTarget;
    beginRenderPassCmd._name = "DO_POSTFX_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::BindPipelineCommand bindPipelineCmd;
    bindPipelineCmd._pipeline = _drawPipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::SendPushConstantsCommand sendPushConstantsCmd;
    _drawConstants.set("_zPlanes", GFX::PushConstantType::VEC2, camera.getZPlanes());
    sendPushConstantsCmd._constants = _drawConstants;
    GFX::EnqueueCommand(bufferInOut, sendPushConstantsCmd);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd;
    bindDescriptorSetsCmd._set._textureData.setTexture(depthData, to_U8(ShaderProgram::TextureUsage::DEPTH));
    bindDescriptorSetsCmd._set._textureData.setTexture(output, to_U8(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN));
    bindDescriptorSetsCmd._set._textureData.setTexture(data0, to_U8(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER));
    bindDescriptorSetsCmd._set._textureData.setTexture(data1, to_U8(TexOperatorBindPoint::TEX_BIND_POINT_NOISE));
    bindDescriptorSetsCmd._set._textureData.setTexture(data2, to_U8(TexOperatorBindPoint::TEX_BIND_POINT_BORDER));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    GFX::DrawCommand drawCommand = { _drawCommand };
    GFX::EnqueueCommand(bufferInOut, drawCommand);

    GFX::EndRenderPassCommand endRenderPassCmd;
    endRenderPassCmd._autoResolveMSAAColour = true;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

void PostFX::idle(const Configuration& config) {
    // Update states
    if (getFilterState(FilterType::FILTER_NOISE)) {
        _noiseTimer += Time::ElapsedMilliseconds();
        if (_noiseTimer > _tickInterval) {
            _noiseTimer = 0.0;
            _randomNoiseCoefficient = Random(1000) * 0.001f;
            _randomFlashCoefficient = Random(1000) * 0.001f;
        }

        _drawConstants.set("randomCoeffNoise", GFX::PushConstantType::FLOAT, _randomNoiseCoefficient);
        _drawConstants.set("randomCoeffNoise", GFX::PushConstantType::FLOAT, _randomFlashCoefficient);
    }

    _preRenderBatch->idle(config);
}

void PostFX::update(const U64 deltaTimeUS) {
    if (_fadeActive) {
        _currentFadeTimeMS += Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);
        F32 fadeStrength = to_F32(std::min(_currentFadeTimeMS / _targetFadeTimeMS , 1.0));
        if (!_fadeOut) {
            fadeStrength = 1.0f - fadeStrength;
        }

        if (fadeStrength > 0.99) {
            if (_fadeWaitDurationMS < EPSILON_D64) {
                if (_fadeOutComplete) {
                    _fadeOutComplete();
                    _fadeOutComplete = DELEGATE_CBK<void>();
                }
            } else {
                _fadeWaitDurationMS -= Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);
            }
        }

        _drawConstants.set("_fadeStrength", GFX::PushConstantType::FLOAT, fadeStrength);
        
        _fadeActive = fadeStrength > EPSILON_D64;
        if (!_fadeActive) {
            _drawConstants.set("_fadeActive", GFX::PushConstantType::BOOL, false);
            if (_fadeInComplete) {
                _fadeInComplete();
                _fadeInComplete = DELEGATE_CBK<void>();
            }
        }
    }
}

void PostFX::setFadeOut(const UColour3& targetColour, D64 durationMS, D64 waitDurationMS, DELEGATE_CBK<void> onComplete) {
    _drawConstants.set("_fadeColour", GFX::PushConstantType::VEC4, Util::ToFloatColour(targetColour));
    _drawConstants.set("_fadeActive", GFX::PushConstantType::BOOL, true);
    _targetFadeTimeMS = durationMS;
    _currentFadeTimeMS = 0.0;
    _fadeWaitDurationMS = waitDurationMS;
    _fadeOut = true;
    _fadeActive = true;
    _fadeOutComplete = onComplete;
}

// clear any fading effect currently active over the specified time interval
// set durationMS to instantly clear the fade effect
void PostFX::setFadeIn(D64 durationMS, DELEGATE_CBK<void> onComplete) {
    _targetFadeTimeMS = durationMS;
    _currentFadeTimeMS = 0.0;
    _fadeOut = false;
    _fadeActive = true;
    _drawConstants.set("_fadeActive", GFX::PushConstantType::BOOL, true);
    _fadeInComplete = onComplete;
}

void PostFX::setFadeOutIn(const UColour3& targetColour, D64 durationFadeOutMS, D64 durationMS) {
    if (durationMS > 0.0) {
        setFadeOutIn(targetColour, durationMS * 0.5, durationMS * 0.5, durationFadeOutMS);
    }
}

void PostFX::setFadeOutIn(const UColour3& targetColour, D64 durationFadeOutMS, D64 durationFadeInMS, D64 waitDurationMS) {
    setFadeOut(targetColour, durationFadeOutMS, waitDurationMS, [this, durationFadeInMS]() {setFadeIn(durationFadeInMS); });
}

};
