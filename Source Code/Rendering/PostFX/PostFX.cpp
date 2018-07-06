#include "Headers/PostFX.h"
#include "Headers/PreRenderOperator.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {

PostFX::PostFX()
    : _enableNoise(false),
      _enableVignette(false),
      _underwater(false),
      _screenBorder(nullptr),
      _noise(nullptr),
      _randomNoiseCoefficient(0.0f),
      _randomFlashCoefficient(0.0f),
      _timer(0.0),
      _tickInterval(1),
      _postProcessingShader(nullptr),
      _underwaterTexture(nullptr),
      _gfx(nullptr)
{
    ParamHandler::instance().setParam<bool>(_ID("postProcessing.enableVignette"), false);

    _postFXTarget._clearDepthBufferOnBind = false;
    _postFXTarget._drawMask.fill(false);
    _postFXTarget._drawMask[0] = true;
}

PostFX::~PostFX()
{
    _preRenderBatch.destroy();
}

void PostFX::init() {
    Console::printfn(Locale::get(_ID("START_POST_FX")));
    ParamHandler& par = ParamHandler::instance();
    _gfx = &GFX_DEVICE;
    _enableNoise = par.getParam<bool>(_ID("postProcessing.enableNoise"));
    _enableVignette = par.getParam<bool>(_ID("postProcessing.enableVignette"));

    ResourceDescriptor postFXShader("postProcessing");
    postFXShader.setThreadedLoading(false);
    postFXShader.setPropertyList(
    Util::StringFormat("TEX_BIND_POINT_SCREEN %d, "
                       "TEX_BIND_POINT_NOISE %d, "
                       "TEX_BIND_POINT_BORDER %d, "
                       "TEX_BIND_POINT_UNDERWATER %d",
                        to_const_uint(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN),
                        to_const_uint(TexOperatorBindPoint::TEX_BIND_POINT_NOISE),
                        to_const_uint(TexOperatorBindPoint::TEX_BIND_POINT_BORDER),
                        to_const_uint(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER)).c_str());

    _postProcessingShader = CreateResource<ShaderProgram>(postFXShader);
    _postProcessingShader->Uniform("_noiseTile", 0.05f);
    _postProcessingShader->Uniform("_noiseFactor", 0.02f);
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "Vignette"));  // 0
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "Noise"));  // 1
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "screenUnderwater"));  // 2
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "screenNormal"));  // 3
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "ColorPassThrough"));  // 4

    _shaderFunctionSelection.resize(
        _postProcessingShader->GetSubroutineUniformCount(ShaderType::FRAGMENT), 0);
    
    ResourceDescriptor textureWaterCaustics("Underwater Caustics");
    textureWaterCaustics.setResourceLocation(
        par.getParam<stringImpl>(_ID("assetsLocation")) +
        "/misc_images/terrain_water_NM.jpg");
    textureWaterCaustics.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
    _underwaterTexture = CreateResource<Texture>(textureWaterCaustics);

     ResourceDescriptor noiseTexture("noiseTexture");
     noiseTexture.setResourceLocation(
            par.getParam<stringImpl>(_ID("assetsLocation")) +
            "/misc_images//bruit_gaussien.jpg");
     noiseTexture.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
     _noise = CreateResource<Texture>(noiseTexture);

     ResourceDescriptor borderTexture("borderTexture");
     borderTexture.setResourceLocation(
            par.getParam<stringImpl>(_ID("assetsLocation")) +
            "/misc_images//vignette.jpeg");
     borderTexture.setEnumValue(to_const_uint(TextureType::TEXTURE_2D));
     _screenBorder = CreateResource<Texture>(borderTexture);

     _preRenderBatch.init(_gfx->getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer);
    PreRenderOperator& SSAOOp = _preRenderBatch.getOperator(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
    SSAOOp.addInputFB(_gfx->getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer);

    PreRenderOperator& DOFOp = _preRenderBatch.getOperator(FilterType::FILTER_DEPTH_OF_FIELD);
    DOFOp.addInputFB(_gfx->getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer);
    
    _timer = 0;
    _tickInterval = 1.0f / 24.0f;
    _randomNoiseCoefficient = 0;
    _randomFlashCoefficient = 0;
}

void PostFX::updateResolution(U16 width, U16 height) {
    if ((_resolutionCache.width == width &&
         _resolutionCache.height == height)|| 
        width < 1 || height < 1)
    {
        return;
    }

    _resolutionCache.set(width, height);

    _preRenderBatch.reshape(width, height);
}

void PostFX::apply() {
    _shaderFunctionSelection[0] = _shaderFunctionList[_enableVignette ? 0 : 4];
    _shaderFunctionSelection[1] = _shaderFunctionList[_enableNoise ? 1 : 4];
    _shaderFunctionSelection[2] = _shaderFunctionList[_underwater ? 2 : 3];

    GFX::Scoped2DRendering scoped2D(true);
    _preRenderBatch.execute(_filterMask);
    _postProcessingShader->bind();
    _postProcessingShader->SetSubroutines(ShaderType::FRAGMENT, _shaderFunctionSelection);

    _preRenderBatch.bindOutput(to_const_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN));
    _underwaterTexture->Bind(to_const_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER));
    _noise->Bind(to_const_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_NOISE));
    _screenBorder->Bind(to_const_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_BORDER));

    _gfx->getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer->begin(_postFXTarget);
        _gfx->drawTriangle(_gfx->getDefaultStateBlock(true), _postProcessingShader);
    _gfx->getRenderTarget(GFXDevice::RenderTargetID::SCREEN)._buffer->end();
}

void PostFX::idle() {
    ParamHandler& par = ParamHandler::instance();
    // Update states
    _underwater = SceneManager::instance().getActiveScene().state().cameraUnderwater();
    _enableNoise = par.getParam<bool>(_ID("postProcessing.enableNoise"));
    _enableVignette = par.getParam<bool>(_ID("postProcessing.enableVignette"));

    if (_enableNoise) {
        _timer += Time::ElapsedMilliseconds();

        if (_timer > _tickInterval) {
            _timer = 0.0;
            _randomNoiseCoefficient = Random(1000) * 0.001f;
            _randomFlashCoefficient = Random(1000) * 0.001f;
        }

        _postProcessingShader->Uniform("randomCoeffNoise",
                                       _randomNoiseCoefficient);
        _postProcessingShader->Uniform("randomCoeffFlash",
                                       _randomFlashCoefficient);
    }

    _preRenderBatch.idle();

    bool enablePostAA = par.getParam<I32>(_ID("rendering.PostAASamples"), 0) > 0;
    bool enableSSR = false;
    bool enableSSAO = par.getParam<bool>(_ID("postProcessing.enableSSAO"), false);
    bool enableDoF = par.getParam<bool>(_ID("postProcessing.enableDepthOfField"), false);
    bool enableMotionBlur = false;
    bool enableBloom = par.getParam<bool>(_ID("postProcessing.enableBloom"), false);
    bool enableLUT = false;

    _filterMask = 0;
    toggleFilter(FilterType::FILTER_SS_ANTIALIASING, enablePostAA);
    toggleFilter(FilterType::FILTER_SS_REFLECTIONS, enableSSR);
    toggleFilter(FilterType::FILTER_SS_AMBIENT_OCCLUSION, enableSSAO);
    toggleFilter(FilterType::FILTER_DEPTH_OF_FIELD, enableDoF);
    toggleFilter(FilterType::FILTER_MOTION_BLUR, enableMotionBlur);
    toggleFilter(FilterType::FILTER_BLOOM, enableBloom);
    toggleFilter(FilterType::FILTER_LUT_CORECTION, enableLUT);
}

};
