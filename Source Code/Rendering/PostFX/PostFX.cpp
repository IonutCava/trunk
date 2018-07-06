#include "Headers/PostFX.h"
#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"
#include "Headers/PreRenderStageBuilder.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Buffers/FrameBuffer/Headers/FrameBuffer.h"

PostFX::PostFX(): _underwaterTexture(nullptr),
    _anaglyphShader(nullptr),
    _postProcessingShader(nullptr),
    _noise(nullptr),
    _screenBorder(nullptr),
    _bloomFB(nullptr),
    _SSAO_FB(nullptr),
    _fxaaOP(nullptr),
    _dofOP(nullptr),
    _bloomOP(nullptr),
    _underwater(false),
    _depthPreview(false),
    _gfx(nullptr)
{
    PreRenderStageBuilder::createInstance();
}

PostFX::~PostFX()
{

    if(_postProcessingShader){
        RemoveResource(_postProcessingShader);
        if(_underwaterTexture)      RemoveResource(_underwaterTexture);
        if(_gfx->anaglyphEnabled()) RemoveResource(_anaglyphShader);
        if(_enableBloom)            SAFE_DELETE(_bloomFB);
        if(_enableSSAO)             SAFE_DELETE(_SSAO_FB);

        if(_enableNoise){
            RemoveResource(_noise);
            RemoveResource(_screenBorder);
        }
    }
    PreRenderStageBuilder::getInstance().destroyInstance();
}

void PostFX::init(const vec2<U16>& resolution){
    _resolutionCache = resolution;

    PRINT_FN(Locale::get("START_POST_FX"));
    ParamHandler& par = ParamHandler::getInstance();
    _gfx = &GFX_DEVICE;
    _enableBloom = par.getParam<bool>("postProcessing.enableBloom");
    _enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");
    _enableFXAA = _gfx->FXAAEnabled();

    if (_gfx->postProcessingEnabled()){
        ResourceDescriptor postFXShader("postProcessing");
        std::stringstream ss;
        if(_enableBloom) ss << "POSTFX_ENABLE_BLOOM,";
        if(_enableSSAO)  ss << "POSTFX_ENABLE_SSAO,";
#ifdef _DEBUG
        ss << "USE_DEPTH_PREVIEW,";
#endif
        ss << "DEFINE_PLACEHOLDER";

        postFXShader.setPropertyList(ss.str());
        postFXShader.setThreadedLoading(false);
        _postProcessingShader = CreateResource<ShaderProgram>(postFXShader);

        ResourceDescriptor textureWaterCaustics("Underwater Caustics");
        textureWaterCaustics.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images/terrain_water_NM.jpg");
        _underwaterTexture = CreateResource<Texture2D>(textureWaterCaustics);

        createOperators();

        _postProcessingShader->Uniform("_noiseTile", 0.05f);
        _postProcessingShader->Uniform("_noiseFactor", 0.02f);
        _postProcessingShader->UniformTexture("texScreen", TEX_BIND_POINT_SCREEN);
        _postProcessingShader->UniformTexture("texBloom", TEX_BIND_POINT_BLOOM);
        _postProcessingShader->UniformTexture("texSSAO", TEX_BIND_POINT_SSAO);
        _postProcessingShader->UniformTexture("texNoise", TEX_BIND_POINT_NOISE);
        _postProcessingShader->UniformTexture("texVignette", TEX_BIND_POINT_BORDER);
        _postProcessingShader->UniformTexture("texWaterNoiseNM", TEX_BIND_POINT_UNDERWATER);
    }

    _timer = 0;
    _tickInterval = 1.0f/24.0f;
    _randomNoiseCoefficient = 0;
    _randomFlashCoefficient = 0;

    updateResolution(resolution.width, resolution.height);

    //par.setParam("postProcessing.enableDepthOfField", false); //enable using keyboard;
}

void PostFX::createOperators(){
    ParamHandler& par = ParamHandler::getInstance();

    PreRenderStageBuilder& stageBuilder = PreRenderStageBuilder::getInstance();
    FrameBuffer* screenBuffer = _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN);
    FrameBuffer* depthBuffer = _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH);

    if (_gfx->anaglyphEnabled()){
        ResourceDescriptor anaglyph("anaglyph");
        anaglyph.setThreadedLoading(false);
        _anaglyphShader = CreateResource<ShaderProgram>(anaglyph);
        _anaglyphShader->UniformTexture("texRightEye", TEX_BIND_POINT_RIGHT_EYE);
        _anaglyphShader->UniformTexture("texLeftEye", TEX_BIND_POINT_LEFT_EYE);
    }
    // Bloom and Ambient Occlusion generate textures that are applied in the PostFX shader
    if(_enableBloom && !_bloomFB){
        _bloomFB = _gfx->newFB();
        _bloomOP = stageBuilder.addPreRenderOperator<BloomPreRenderOperator>(_enableBloom, _bloomFB, _resolutionCache);
        _bloomOP->addInputFB(screenBuffer);
    }

    if(_enableSSAO && !_SSAO_FB){
        _SSAO_FB = _gfx->newFB();
        PreRenderOperator* ssaoOP = stageBuilder.addPreRenderOperator<SSAOPreRenderOperator>(_enableSSAO, _SSAO_FB, _resolutionCache);
        ssaoOP->addInputFB(screenBuffer);
        ssaoOP->addInputFB(depthBuffer);
    }

    // DOF and FXAA modify the screen FB
    if(_enableDOF && !_dofOP){
        _dofOP = stageBuilder.addPreRenderOperator<DoFPreRenderOperator>(_enableDOF, screenBuffer, _resolutionCache);
        _dofOP->addInputFB(screenBuffer);
        _dofOP->addInputFB(depthBuffer);
    }

    if(_enableFXAA && !_fxaaOP){
        _fxaaOP = stageBuilder.addPreRenderOperator<FXAAPreRenderOperator>(_enableFXAA, screenBuffer, _resolutionCache);
        _fxaaOP->addInputFB(screenBuffer);
    }

    if(_enableNoise && !_noise){
        ResourceDescriptor noiseTexture("noiseTexture");
        ResourceDescriptor borderTexture("borderTexture");
        noiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//bruit_gaussien.jpg");
        borderTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//vignette.jpeg");
        _noise = CreateResource<Texture>(noiseTexture);
        _screenBorder = CreateResource<Texture>(borderTexture);
    }
}

void PostFX::updateResolution(I32 width, I32 height){
    if (!_gfx || !_gfx->postProcessingEnabled()) return;

    if (vec2<U16>(width, height) == _resolutionCache || width < 1 || height < 1)
        return;

    _resolutionCache.set(width, height);
    _postProcessingShader->refresh();
    PreRenderStageBuilder::getInstance().getPreRenderBatch()->reshape(width,height);
}

void PostFX::displaySceneAnaglyph(){
    _gfx->toggle2D(true);
    _anaglyphShader->bind();
    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Bind(TEX_BIND_POINT_RIGHT_EYE); //right eye buffer
    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_ANAGLYPH)->Bind(TEX_BIND_POINT_LEFT_EYE); //left eye buffer

       _gfx->drawPoints(1);

    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_ANAGLYPH)->Unbind(TEX_BIND_POINT_LEFT_EYE);
    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Unbind(TEX_BIND_POINT_RIGHT_EYE);
    _gfx->toggle2D(false);
}

void PostFX::displayScene(){
    if (!_gfx->postProcessingEnabled())
         return;
    if (_gfx->anaglyphEnabled()){
        displaySceneAnaglyph();
        return;
    }
    _gfx->toggle2D(true);

    PreRenderStageBuilder::getInstance().getPreRenderBatch()->execute();

    _postProcessingShader->bind();
#ifdef _DEBUG
    if (_depthPreview) _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Bind(TEX_BIND_POINT_SCREEN, TextureDescriptor::Depth);
    else               _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Bind(TEX_BIND_POINT_SCREEN, TextureDescriptor::Color0);

    _postProcessingShader->Uniform("enable_depth_preview", _depthPreview);
#else
    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Bind(TEX_BIND_POINT_SCREEN);
#endif

    bool underwater = (_underwater && _underwaterTexture);   

    _postProcessingShader->Uniform("enable_underwater", underwater);
    
    if (underwater)  _underwaterTexture->Bind(TEX_BIND_POINT_UNDERWATER);
    if(_enableBloom) _bloomFB->Bind(TEX_BIND_POINT_BLOOM);
    if(_enableSSAO)  _SSAO_FB->Bind(TEX_BIND_POINT_SSAO);
    if(_enableNoise){
        _noise->Bind(TEX_BIND_POINT_NOISE);
        _screenBorder->Bind(TEX_BIND_POINT_BORDER);
    }

    _gfx->drawPoints(1);

    if(_enableNoise){
        _screenBorder->Unbind(TEX_BIND_POINT_BORDER);
        _noise->Unbind(TEX_BIND_POINT_NOISE);
    }
    if(_enableSSAO)  _SSAO_FB->Unbind(TEX_BIND_POINT_SSAO);
    if(_enableBloom) _bloomFB->Unbind(TEX_BIND_POINT_BLOOM);
    if(underwater)   _underwaterTexture->Unbind(TEX_BIND_POINT_UNDERWATER);

#ifdef NDEBUG
    _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Unbind(TEX_BIND_POINT_SCREEN);
#else
    if (_depthPreview) _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Unbind(TEX_BIND_POINT_SCREEN);
    else               _gfx->getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Unbind(TEX_BIND_POINT_SCREEN);
#endif

    _gfx->toggle2D(false);
}

void PostFX::idle(){
    ParamHandler& par = ParamHandler::getInstance();
    //Update states
    bool recompileShader = false;

    if (!_gfx->postProcessingEnabled() || !_postProcessingShader)
        return;

    _underwater = par.getParam<bool>("scene.camera.underwater");
    _enableDOF  = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");
    _enableFXAA = _gfx->FXAAEnabled();

    if(_enableBloom != par.getParam<bool>("postProcessing.enableBloom")){
        _enableBloom = !_enableBloom;
        if(_enableBloom)
            _postProcessingShader->addShaderDefine("POSTFX_ENABLE_BLOOM");
        else
            _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_BLOOM");
        recompileShader = true;
    }

    if(_enableSSAO != par.getParam<bool>("postProcessing.enableSSAO")){
        _enableSSAO = !_enableSSAO;
        if(_enableSSAO)
            _postProcessingShader->addShaderDefine("POSTFX_ENABLE_SSAO");
        else
            _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_SSAO");
        recompileShader = true;
    }

    createOperators();

    //recreate only the fragment shader
    if (recompileShader){
        _postProcessingShader->recompile(false,true);
        _postProcessingShader->UniformTexture("texScreen", TEX_BIND_POINT_SCREEN);
        _postProcessingShader->UniformTexture("texBloom", TEX_BIND_POINT_BLOOM);
        _postProcessingShader->UniformTexture("texSSAO", TEX_BIND_POINT_SSAO);
        _postProcessingShader->UniformTexture("texNoise", TEX_BIND_POINT_NOISE);
        _postProcessingShader->UniformTexture("texVignette", TEX_BIND_POINT_BORDER);
        _postProcessingShader->UniformTexture("texWaterNoiseNM", TEX_BIND_POINT_UNDERWATER);
    }

    _postProcessingShader->bind();

    if(_enableBloom)
        _postProcessingShader->Uniform("bloomFactor", par.getParam<F32>("postProcessing.bloomFactor"));

    _postProcessingShader->Uniform("enableVignette",_enableNoise);
    _postProcessingShader->Uniform("enableNoise",   _enableNoise);

    if(_enableNoise){
        _timer += GETMSTIME();
        if(_timer > _tickInterval ){
            _timer = 0.0;
            _randomNoiseCoefficient = (F32)random(1000) * 0.001f;
            _randomFlashCoefficient = (F32)random(1000) * 0.001f;
        }

        _postProcessingShader->Uniform("randomCoeffNoise", _randomNoiseCoefficient);
        _postProcessingShader->Uniform("randomCoeffFlash", _randomFlashCoefficient);
    }

    _postProcessingShader->unbind();
}