#include "Headers/PostFX.h"
#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"
#include "Headers/PreRenderStageBuilder.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {

PostFX::PostFX()
    : _enableBloom(false),
      _enableDOF(false),
      _enableNoise(false),
      _enableVignette(false),
      _enableSSAO(false),
      _enableFXAA(false),
      _underwater(false),
      _depthPreview(false),
      _bloomFB(nullptr),
      _bloomOP(nullptr),
      _SSAO_FB(nullptr),
      _fxaaOP(nullptr),
      _dofOP(nullptr),
      _screenBorder(nullptr),
      _noise(nullptr),
      _randomNoiseCoefficient(0.0f),
      _randomFlashCoefficient(0.0f),
      _timer(0.0),
      _tickInterval(1),
      _anaglyphShader(nullptr),
      _postProcessingShader(nullptr),
      _underwaterTexture(nullptr),
      _gfx(nullptr)
{
    PreRenderStageBuilder::createInstance();
    ParamHandler::getInstance().setParam<bool>("postProcessing.enableVignette", false);
    ParamHandler::getInstance().setParam<bool>("postProcessing.fullScreenDepthBuffer", false);
}

PostFX::~PostFX()
{
    RemoveResource(_postProcessingShader);
    RemoveResource(_anaglyphShader);
    RemoveResource(_underwaterTexture);
    RemoveResource(_noise);
    RemoveResource(_screenBorder);

    MemoryManager::DELETE(_bloomFB);
    MemoryManager::DELETE(_SSAO_FB);

    PreRenderStageBuilder::destroyInstance();
}

void PostFX::init(const vec2<U16>& resolution) {
    Console::printfn(Locale::get("START_POST_FX"));
    ParamHandler& par = ParamHandler::getInstance();
    _gfx = &GFX_DEVICE;
    _enableBloom = par.getParam<bool>("postProcessing.enableBloom");
    _enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");
    _enableVignette = par.getParam<bool>("postProcessing.enableVignette");
    _enableFXAA = _gfx->gpuState().FXAAEnabled();

    ResourceDescriptor anaglyph("anaglyph");
    anaglyph.setThreadedLoading(false);
    _anaglyphShader = CreateResource<ShaderProgram>(anaglyph);
    _anaglyphShader->Uniform("texRightEye", to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_RIGHT_EYE));
    _anaglyphShader->Uniform("texLeftEye", to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_LEFT_EYE));

    ResourceDescriptor postFXShader("postProcessing");
    postFXShader.setThreadedLoading(false);
    postFXShader.setPropertyList(
    Util::StringFormat("TEX_BIND_POINT_SCREEN %d, "
                       "TEX_BIND_POINT_BLOOM %d, "
                       "TEX_BIND_POINT_SSAO %d, "
                       "TEX_BIND_POINT_NOISE %d, "
                       "TEX_BIND_POINT_BORDER %d, "
                       "TEX_BIND_POINT_UNDERWATER %d",
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN),
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_BLOOM),
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_SSAO),
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_NOISE),
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_BORDER),
                        to_uint(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER)).c_str());

    _postProcessingShader = CreateResource<ShaderProgram>(postFXShader);
    _postProcessingShader->Uniform("_noiseTile", 0.05f);
    _postProcessingShader->Uniform("_noiseFactor", 0.02f);
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "Vignette"));  // 0
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "Noise"));  // 1
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "Bloom"));  // 2
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "SSAO"));  // 3
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "screenUnderwater"));  // 4
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "screenNormal"));  // 5
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "ColorPassThrough"));  // 6
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "outputScreen"));  // 7
    _shaderFunctionList.push_back(_postProcessingShader->GetSubroutineIndex(
            ShaderType::FRAGMENT, "outputDepth"));  // 8
    _shaderFunctionSelection.resize(
        _postProcessingShader->GetSubroutineUniformCount(ShaderType::FRAGMENT), 0);
    
    ResourceDescriptor textureWaterCaustics("Underwater Caustics");
    textureWaterCaustics.setResourceLocation(
        par.getParam<stringImpl>("assetsLocation") +
        "/misc_images/terrain_water_NM.jpg");
    _underwaterTexture = CreateResource<Texture>(textureWaterCaustics);

     ResourceDescriptor noiseTexture("noiseTexture");
     noiseTexture.setResourceLocation(
            par.getParam<stringImpl>("assetsLocation") +
            "/misc_images//bruit_gaussien.jpg");
     _noise = CreateResource<Texture>(noiseTexture);

     ResourceDescriptor borderTexture("borderTexture");
     borderTexture.setResourceLocation(
            par.getParam<stringImpl>("assetsLocation") +
            "/misc_images//vignette.jpeg");
     _screenBorder = CreateResource<Texture>(borderTexture);

    _timer = 0;
    _tickInterval = 1.0f / 24.0f;
    _randomNoiseCoefficient = 0;
    _randomFlashCoefficient = 0;

    updateResolution(resolution.width, resolution.height);
}

void PostFX::updateOperators() {
    PreRenderStageBuilder& stageBuilder = PreRenderStageBuilder::getInstance();
    Framebuffer* screenBuffer = _gfx->getRenderTarget(GFXDevice::RenderTarget::SCREEN);
    Framebuffer* depthBuffer =  _gfx->getRenderTarget(GFXDevice::RenderTarget::DEPTH);

    if (_enableBloom && !_bloomFB) {
        _bloomFB = _gfx->newFB();
        _bloomOP = stageBuilder.addPreRenderOperator<BloomPreRenderOperator>(
                       _enableBloom, _bloomFB, _resolutionCache);
        _bloomOP->addInputFB(screenBuffer);
        //_bloomOP->genericFlag(toneMapEnabled);
    }

    if (_enableSSAO && !_SSAO_FB) {
        _SSAO_FB = _gfx->newFB();
        PreRenderOperator* ssaoOP = nullptr;
        ssaoOP = stageBuilder.addPreRenderOperator<SSAOPreRenderOperator>(
                    _enableSSAO, _SSAO_FB, _resolutionCache);
        ssaoOP->addInputFB(screenBuffer);
        ssaoOP->addInputFB(depthBuffer);
    }

    // DOF and FXAA modify the screen FB
    if (_enableDOF && !_dofOP) {
        _dofOP = stageBuilder.addPreRenderOperator<DoFPreRenderOperator>(
                     _enableDOF, screenBuffer, _resolutionCache);
        _dofOP->addInputFB(screenBuffer);
        _dofOP->addInputFB(depthBuffer);
    }

    if (_enableFXAA && !_fxaaOP) {
        _fxaaOP = stageBuilder.addPreRenderOperator<FXAAPreRenderOperator>(
                      _enableFXAA, screenBuffer, _resolutionCache);
        _fxaaOP->addInputFB(screenBuffer);
    }
}

void PostFX::updateResolution(U16 width, U16 height) {
    if (!_gfx || !_gfx->postProcessingEnabled()) {
        return;
    }

    if ((_resolutionCache.width == width &&
         _resolutionCache.height == height)|| 
        width < 1 || height < 1)
    {
        return;
    }

    _resolutionCache.set(width, height);

    updateOperators();
    PreRenderStageBuilder::getInstance().getPreRenderBatch()->reshape(width, height);
}

void PostFX::displayScene(bool applyFilters) {
    updateShaderStates(applyFilters);

    GFX::Scoped2DRendering scoped2D(true);
    ShaderProgram* drawShader = nullptr;
    if (_gfx->anaglyphEnabled()) {
        STUBBED("ToDo: Add PostFX support for anaglyph rendering! -Ionut")

        drawShader = _anaglyphShader;
        _gfx->getRenderTarget(GFXDevice::RenderTarget::SCREEN)
            ->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_RIGHT_EYE));

        _gfx->getRenderTarget(GFXDevice::RenderTarget::ANAGLYPH)
            ->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_LEFT_EYE));
    } else {
        if (applyFilters) {
            PreRenderStageBuilder::getInstance().getPreRenderBatch()->execute();
        }

        drawShader = _postProcessingShader;
        _postProcessingShader->bind();
        _postProcessingShader->SetSubroutines(ShaderType::FRAGMENT, _shaderFunctionSelection);

#ifdef _DEBUG
        _gfx->getRenderTarget(
                  _depthPreview ? GFXDevice::RenderTarget::DEPTH
                                : GFXDevice::RenderTarget::SCREEN)
            ->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN),
                   _depthPreview ? TextureDescriptor::AttachmentType::Depth
                                 : TextureDescriptor::AttachmentType::Color0);
#else
        _gfx->getRenderTarget(GFXDevice::RenderTarget::SCREEN)
            ->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN));
#endif
        if (applyFilters) {
            _underwaterTexture->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER));
            _noise->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_NOISE));
            _screenBorder->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_BORDER));
            if (_bloomFB) {
                _bloomFB->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_BLOOM));
            }
            if (_SSAO_FB) {
                _SSAO_FB->Bind(to_ubyte(TexOperatorBindPoint::TEX_BIND_POINT_SSAO));
            }
        }
    }

    _gfx->drawTriangle(_gfx->getDefaultStateBlock(true), drawShader);
}

void PostFX::idle() {
    if (!_gfx || !_gfx->postProcessingEnabled()) {
        return;
    }

    ParamHandler& par = ParamHandler::getInstance();
    // Update states
    _underwater = GET_ACTIVE_SCENE().state().cameraUnderwater();
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");
    _enableVignette = par.getParam<bool>("postProcessing.enableVignette");
    _depthPreview = par.getParam<bool>("postProcessing.fullScreenDepthBuffer");
    _enableFXAA = _gfx->gpuState().FXAAEnabled();

    bool recompileShader = false;
    if (_enableBloom != par.getParam<bool>("postProcessing.enableBloom")) {
        _enableBloom = !_enableBloom;
        if (_enableBloom) {
            _postProcessingShader->addShaderDefine("POSTFX_ENABLE_BLOOM");
        } else {
            _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_BLOOM");
        }
        recompileShader = true;
    }

    if (_enableSSAO != par.getParam<bool>("postProcessing.enableSSAO")) {
        _enableSSAO = !_enableSSAO;
        if (_enableSSAO) {
            _postProcessingShader->addShaderDefine("POSTFX_ENABLE_SSAO");
        } else {
            _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_SSAO");
        }
        recompileShader = true;
    }

    // recreate only the fragment shader
    if (recompileShader) {
        _postProcessingShader->recompile(false, true);
        _postProcessingShader->Uniform(
            "texScreen", to_int(TexOperatorBindPoint::TEX_BIND_POINT_SCREEN));
        _postProcessingShader->Uniform(
            "texBloom", to_int(TexOperatorBindPoint::TEX_BIND_POINT_BLOOM));
        _postProcessingShader->Uniform(
            "texSSAO", to_int(TexOperatorBindPoint::TEX_BIND_POINT_SSAO));
        _postProcessingShader->Uniform(
            "texNoise", to_int(TexOperatorBindPoint::TEX_BIND_POINT_NOISE));
        _postProcessingShader->Uniform(
            "texVignette", to_int(TexOperatorBindPoint::TEX_BIND_POINT_BORDER));
        _postProcessingShader->Uniform(
            "texWaterNoiseNM",
            to_int(TexOperatorBindPoint::TEX_BIND_POINT_UNDERWATER));
    }

    if (_enableBloom) {
        _postProcessingShader->Uniform(
            "bloomFactor", par.getParam<F32>("postProcessing.bloomFactor"));
    }

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

    updateOperators();
}

void PostFX::updateShaderStates(bool applyFilters) {
    _shaderFunctionSelection[0] = _shaderFunctionList[(_enableVignette && applyFilters) ? 0 : 6];
    _shaderFunctionSelection[1] = _shaderFunctionList[(_enableNoise && applyFilters) ? 1 : 6];
    _shaderFunctionSelection[2] = _shaderFunctionList[(_enableBloom && applyFilters) ? 2 : 6];
    _shaderFunctionSelection[3] = _shaderFunctionList[(_enableSSAO && applyFilters) ? 3 : 6];
    _shaderFunctionSelection[4] = _shaderFunctionList[_underwater ? 4 : 5];
    _shaderFunctionSelection[5] = _shaderFunctionList[_depthPreview ? 8 : 7];
}

};
