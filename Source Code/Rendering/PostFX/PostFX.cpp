#include "Headers/PostFX.h"
#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"
#include "Headers/PreRenderStageBuilder.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

PostFX::PostFX(): _underwaterTexture(nullptr),
    _renderQuad(nullptr),
    _anaglyphShader(nullptr),
    _postProcessingShader(nullptr),
    _noise(nullptr),
    _screenBorder(nullptr),
    _bloomFBO(nullptr),
    _SSAO_FBO(nullptr),
    _fxaaOP(nullptr),
    _dofOP(nullptr),
    _bloomOP(nullptr),
    _underwater(false),
    _depthPreview(false),
    _gfx(GFX_DEVICE)
{
    PreRenderStageBuilder::createInstance();
}

PostFX::~PostFX()
{
    if(_renderQuad)
        RemoveResource(_renderQuad);

    if(_postProcessingShader){
        if(_underwaterTexture)
            RemoveResource(_underwaterTexture);

        RemoveResource(_postProcessingShader);

        if(_gfx.anaglyphEnabled()){
            RemoveResource(_anaglyphShader);
        }

        if(_enableBloom){
            if(_enableBloom)
                SAFE_DELETE(_bloomFBO);
        }

        if(_enableNoise){
            RemoveResource(_noise);
            RemoveResource(_screenBorder);
        }

        if(_enableSSAO)
            SAFE_DELETE(_SSAO_FBO);
    }
    PreRenderStageBuilder::getInstance().destroyInstance();
}

void PostFX::init(const vec2<U16>& resolution){
    PRINT_FN(Locale::get("START_POST_FX"));
    ParamHandler& par = ParamHandler::getInstance();

    _enableBloom = par.getParam<bool>("postProcessing.enableBloom");
    _enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");
    _enableFXAA = par.getParam<bool>("postProcessing.enableFXAA");

    if(_gfx.postProcessingEnabled()){
        ResourceDescriptor mrt("PostFX RenderQuad");
        mrt.setFlag(true); //No default Material;
        _renderQuad = CreateResource<Quad3D>(mrt);
        assert(_renderQuad);
        _renderQuad->setDimensions(vec4<F32>(0,0,resolution.width,resolution.height));
        _renderQuad->renderInstance()->preDraw(true);
        _renderQuad->renderInstance()->draw2D(true);
        ResourceDescriptor postFXShader("postProcessing");
        std::stringstream ss;
        if(_enableBloom) ss << "POSTFX_ENABLE_BLOOM,";
        if(_enableSSAO)  ss << "POSTFX_ENABLE_SSAO,";
#ifdef _DEBUG
        ss << "USE_DEPTH_PREVIEW,";
#endif
        ss << "DEFINE_PLACEHOLDER";

        postFXShader.setPropertyList(ss.str());

        _postProcessingShader = CreateResource<ShaderProgram>(postFXShader);

        ResourceDescriptor textureWaterCaustics("Underwater Caustics");
        textureWaterCaustics.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images/terrain_water_NM.jpg");
        _underwaterTexture = CreateResource<Texture2D>(textureWaterCaustics);

        createOperators();

        _postProcessingShader->Uniform("_noiseTile", 0.05f);
        _postProcessingShader->Uniform("_noiseFactor", 0.02f);
    }

    _timer = 0;
    _tickInterval = 1.0f/24.0f;
    _randomNoiseCoefficient = 0;
    _randomFlashCoefficient = 0;

    reshapeFBO(resolution.width, resolution.height);

    //par.setParam("postProcessing.enableDepthOfField", false); //enable using keyboard;
}

void PostFX::createOperators(){
    ParamHandler& par = ParamHandler::getInstance();

    vec2<U16> resolution(par.getParam<U16>("runtime.resolutionWidth"),
                         par.getParam<U16>("runtime.resolutionHeight"));
    PreRenderStageBuilder& stageBuilder = PreRenderStageBuilder::getInstance();

    if(_gfx.anaglyphEnabled()){
        _anaglyphShader = CreateResource<ShaderProgram>(ResourceDescriptor("anaglyph"));
    }
    // Bloom and Ambient Occlusion generate textures that are applied in the PostFX shader
    if(_enableBloom && !_bloomFBO){
        _bloomFBO = _gfx.newFBO(FBO_2D_COLOR);
        _bloomOP = stageBuilder.addPreRenderOperator<BloomPreRenderOperator>(_renderQuad,_enableBloom,_bloomFBO,resolution);
        _bloomOP->addInputFBO(_gfx.getScreenBuffer(0));
    }

    if(_enableSSAO && !_SSAO_FBO){
        _SSAO_FBO = _gfx.newFBO(FBO_2D_COLOR);
        PreRenderOperator* ssaoOP = stageBuilder.addPreRenderOperator<SSAOPreRenderOperator>(_renderQuad,_enableSSAO, _SSAO_FBO,resolution);
        ssaoOP->addInputFBO(_gfx.getScreenBuffer(0));
        ssaoOP->addInputFBO(_gfx.getDepthBuffer());
    }

    // DOF and FXAA modify the screen FBO
    if(_enableDOF && !_dofOP){
        _dofOP = stageBuilder.addPreRenderOperator<DoFPreRenderOperator>(_renderQuad,_enableDOF, _gfx.getScreenBuffer(0), resolution);
        _dofOP->addInputFBO(_gfx.getScreenBuffer(0));
        _dofOP->addInputFBO(_gfx.getDepthBuffer());
    }

    if(_enableFXAA && !_fxaaOP){
        _fxaaOP = stageBuilder.addPreRenderOperator<FXAAPreRenderOperator>(_renderQuad,_enableFXAA, _gfx.getScreenBuffer(0), resolution);
        _fxaaOP->addInputFBO(_gfx.getScreenBuffer(0));
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

void PostFX::reshapeFBO(I32 width , I32 height){
    if(!_gfx.postProcessingEnabled() || width <= 0 || height <= 0) return;
    if(width == _gfx.getScreenBuffer(0)->getWidth() && height == _gfx.getScreenBuffer(0)->getHeight()) return;

    _renderQuad->setDimensions(vec4<F32>(0,0,width,height));
    PreRenderStageBuilder::getInstance().getPreRenderBatch()->reshape(width,height);
}

void PostFX::displaySceneWithAnaglyph(){
    _anaglyphShader->bind();
    
    _gfx.getScreenBuffer(0)->Bind(0); //right eye buffer
    _gfx.getScreenBuffer(1)->Bind(1); //left eye buffer

        _anaglyphShader->UniformTexture("texRightEye", 0);
        _anaglyphShader->UniformTexture("texLeftEye", 1);

        _gfx.toggle2D(true);
            _renderQuad->setCustomShader(_anaglyphShader);
            _gfx.renderInstance(_renderQuad->renderInstance());
        _gfx.toggle2D(false);

    _gfx.getScreenBuffer(1)->Unbind(1);
    _gfx.getScreenBuffer(0)->Unbind(0);
}

void PostFX::displaySceneWithoutAnaglyph(){
    PreRenderStageBuilder::getInstance().getPreRenderBatch()->execute();

    I32 id = 0;
    _postProcessingShader->bind();
#ifdef _DEBUG
    if(_depthPreview) _gfx.getDepthBuffer()->Bind(id, TextureDescriptor::Depth);
    else              _gfx.getScreenBuffer(0)->Bind(id, TextureDescriptor::Color0);

    _postProcessingShader->Uniform("enable_depth_preview", _depthPreview);
#else
    _gfx.getScreenBuffer(0)->Bind(id);
#endif

    _postProcessingShader->UniformTexture("texScreen", id++);

    if(_underwater && _underwaterTexture){
        _underwaterTexture->Bind(id);
        _postProcessingShader->UniformTexture("texWaterNoiseNM", id++);
    }

    _postProcessingShader->Uniform("enable_underwater",_underwater);
    
    if(_enableBloom){
        _bloomFBO->Bind(id);
        _postProcessingShader->UniformTexture("texBloom", id++);
    }

    if(_enableSSAO){
        _SSAO_FBO->Bind(id);
        _postProcessingShader->UniformTexture("texSSAO", id++);
    }

    if(_enableNoise){
        _noise->Bind(id);
        _postProcessingShader->UniformTexture("texNoise", id++);
        _screenBorder->Bind(id);
        _postProcessingShader->UniformTexture("texVignette", id++);
    }

    _gfx.toggle2D(true);
        _renderQuad->setCustomShader(_postProcessingShader);
        _gfx.renderInstance(_renderQuad->renderInstance());
    _gfx.toggle2D(false);

    if(_enableNoise){
        _screenBorder->Unbind(--id);
        _noise->Unbind(--id);
    }

    if(_enableSSAO){
        _SSAO_FBO->Unbind(--id);
    }
    
    if(_enableBloom){
        _bloomFBO->Unbind(--id);
    }

    if(_underwater && _underwaterTexture){
        _underwaterTexture->Unbind(--id);
    }

    _gfx.getScreenBuffer(0)->Unbind(--id);
}

void PostFX::idle(){
    ParamHandler& par = ParamHandler::getInstance();
    //Update states
    bool recompileShader = false;

    if(!_gfx.postProcessingEnabled() || !_postProcessingShader)
        return;

    _underwater = par.getParam<bool>("scene.camera.underwater");
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableFXAA = par.getParam<bool>("postProcessing.enableFXAA");
    
    _enableNoise = par.getParam<bool>("postProcessing.enableNoise");

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
    if(recompileShader)
        _postProcessingShader->recompile(false,true);

    _postProcessingShader->bind();

    if(_enableBloom)
        _postProcessingShader->Uniform("bloomFactor", par.getParam<F32>("postProcessing.bloomFactor"));

    _postProcessingShader->Uniform("enableVignette",_enableNoise);
    _postProcessingShader->Uniform("enableNoise",_enableNoise);

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