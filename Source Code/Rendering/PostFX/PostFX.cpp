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

PostFX::PostFX(): _underwaterTexture(NULL),
	_renderQuad(NULL),
	_anaglyphShader(NULL),
	_postProcessingShader(NULL),
	_noise(NULL),
	_screenBorder(NULL),
	_bloomFBO(NULL),
	_depthOfFieldFBO(NULL),
	_depthFBO(NULL),
	_screenFBO(NULL),
	_SSAO_FBO(NULL),
	_currentCamera(NULL),
	_underwater(false),
	_FXAAinit(false),
	_gfx(GFX_DEVICE)
{
	PreRenderStageBuilder::createInstance();
	_anaglyphFBO[0] = _anaglyphFBO[1] = NULL;
}

PostFX::~PostFX()
{
	if(_renderQuad)
		RemoveResource(_renderQuad);
	
	if(_enablePostProcessing){

		if(_underwaterTexture)
			RemoveResource(_underwaterTexture);

		SAFE_DELETE(_screenFBO);
		SAFE_DELETE(_depthFBO);

		RemoveResource(_postProcessingShader);

		if(_enableAnaglyph){
			RemoveResource(_anaglyphShader);
			SAFE_DELETE(_anaglyphFBO[0]);
			SAFE_DELETE(_anaglyphFBO[1]);
		}

		if(_enableDOF || _enableBloom){
			if(_enableBloom)
				SAFE_DELETE(_bloomFBO);
			if(_enableDOF)
				SAFE_DELETE(_depthOfFieldFBO);
		}

		if(_enableNoise){
			RemoveResource(_noise);
			RemoveResource(_screenBorder);
		}

		if(_enableSSAO)
			SAFE_DELETE(_SSAO_FBO);
	}
	PreRenderStageBuilder::getInstance().DestroyInstance();
}

void PostFX::init(const vec2<U16>& resolution){
	PRINT_FN(Locale::get("START_POST_FX"));
	ParamHandler& par = ParamHandler::getInstance();

	_enablePostProcessing = par.getParam<bool>("postProcessing.enablePostFX");
	_enableAnaglyph = par.getParam<bool>("postProcessing.enable3D");
	_enableBloom = par.getParam<bool>("postProcessing.enableBloom");
	_enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
	_enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
	_enableNoise = par.getParam<bool>("postProcessing.enableNoise");
	_enableFXAA = par.getParam<bool>("postProcessing.enableFXAA");
    _enableHDR = par.getParam<bool>("rendering.enableHDR");

	if(_enablePostProcessing){
		//Screen FBO should use MSAA if available, else fallback to normal color FBO (no AA or FXAA)
		_screenFBO = _gfx.newFBO(FBO_2D_COLOR_MS);
		_depthFBO  = _gfx.newFBO(FBO_2D_DEPTH);

		SamplerDescriptor screenSampler;
	    TextureDescriptor screenDescriptor(TEXTURE_2D,
										   RGBA,
										   RGBA8,
										   UNSIGNED_BYTE); //Default filters, LINEAR is OK for this

		screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
		screenSampler.toggleMipMaps(false); //it's a flat texture on a full screen quad. really?
		screenDescriptor.setSampler(screenSampler);

		_screenFBO->AddAttachment(screenDescriptor,TextureDescriptor::Color0);
		_screenFBO->toggleDepthBuffer(true);//Do we need a depth buffer?

	    TextureDescriptor depthDescriptor(TEXTURE_2D,
										  DEPTH_COMPONENT,
										  DEPTH_COMPONENT,
										  UNSIGNED_BYTE); ///Default filters, LINEAR is OK for this
		screenSampler._useRefCompare = true; //< Use compare function
		screenSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
		screenSampler._depthCompareMode = LUMINANCE;
		depthDescriptor.setSampler(screenSampler);

		_depthFBO->AddAttachment(depthDescriptor, TextureDescriptor::Depth);
		_depthFBO->toggleColorWrites(false);

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
        if(_enableHDR)   ss << "POSTFX_ENABLE_HDR,";
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

	PostFX::getInstance().reshapeFBO(resolution.width, resolution.height);

	//par.setParam("postProcessing.enableDepthOfField", false); //enable using keyboard;
}

void PostFX::createOperators(){
	ParamHandler& par = ParamHandler::getInstance();

	if(_enableAnaglyph && !_anaglyphFBO[0]){
		SamplerDescriptor screenSampler;
		TextureDescriptor screenDescriptor(TEXTURE_2D,
										   RGBA,
										   RGBA8,
										   FLOAT_32); ///Default filters, LINEAR is OK for this

		screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
		screenSampler.toggleMipMaps(false); //it's a flat texture on a full screen quad. really?
		screenDescriptor.setSampler(screenSampler);

		_anaglyphFBO[0] = _gfx.newFBO(FBO_2D_COLOR_MS);
		_anaglyphFBO[1] = _gfx.newFBO(FBO_2D_COLOR_MS);
		_anaglyphFBO[0]->AddAttachment(screenDescriptor,TextureDescriptor::Color0);
		_anaglyphFBO[1]->AddAttachment(screenDescriptor,TextureDescriptor::Color0);
		_anaglyphFBO[0]->toggleDepthBuffer(true);//Do we need a depth buffer?
		_anaglyphFBO[1]->toggleDepthBuffer(true);//Do we need a depth buffer?
		_anaglyphShader = CreateResource<ShaderProgram>(ResourceDescriptor("anaglyph"));
		_eyeOffset = par.getParam<F32>("postProcessing.anaglyphOffset");
	}
	vec2<U16> resolution(par.getParam<U16>("runtime.resolutionWidth"),par.getParam<U16>("runtime.resolutionHeight"));
	if(_enableBloom && !_bloomFBO){
		_bloomFBO = _gfx.newFBO(FBO_2D_COLOR);
		PreRenderOperator* bloomOp = PreRenderStageBuilder::getInstance().addPreRenderOperator<BloomPreRenderOperator>(_renderQuad,_enableBloom,_bloomFBO,resolution);
		bloomOp->addInputFBO(_screenFBO);
	}
	if(_enableSSAO && !_SSAO_FBO){
		_SSAO_FBO = _gfx.newFBO(FBO_2D_COLOR);
		PreRenderStageBuilder::getInstance().addPreRenderOperator<SSAOPreRenderOperator>(_renderQuad,_enableSSAO, _SSAO_FBO,resolution);
	}
	if(_enableDOF && !_depthOfFieldFBO){
		_depthOfFieldFBO = _gfx.newFBO(FBO_2D_COLOR);
		PreRenderOperator* dofOp = PreRenderStageBuilder::getInstance().addPreRenderOperator<DoFPreRenderOperator>(_renderQuad,_enableDOF, _depthOfFieldFBO, resolution);
		dofOp->addInputFBO(_screenFBO);
		dofOp->addInputFBO(_depthFBO);
	}
	if(_enableNoise && !_noise){
		ResourceDescriptor noiseTexture("noiseTexture");
		ResourceDescriptor borderTexture("borderTexture");
		noiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//bruit_gaussien.jpg");
		borderTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//vignette.jpeg");
		_noise = CreateResource<Texture>(noiseTexture);
		_screenBorder = CreateResource<Texture>(borderTexture);
	}
	if(_enableFXAA && !_FXAAinit){
		PreRenderOperator* fxaaOp = PreRenderStageBuilder::getInstance().addPreRenderOperator<FXAAPreRenderOperator>(_renderQuad,_enableFXAA, _screenFBO, resolution);
		fxaaOp->addInputFBO(_screenFBO);
		_FXAAinit = true;
	}
}

void PostFX::reshapeFBO(I32 width , I32 height){
	if(!_enablePostProcessing || width <= 0 || height <= 0) return;
	if(width == _screenFBO->getWidth() && height == _screenFBO->getHeight()) return;
	_screenFBO->Create(width, height);
	_depthFBO->Create(width, height);
	_renderQuad->setDimensions(vec4<F32>(0,0,width,height));

	if(_enableAnaglyph){
		_anaglyphFBO[0]->Create(width, height);
		_anaglyphFBO[1]->Create(width, height);
	}
	PreRenderStage* renderBatch = PreRenderStageBuilder::getInstance().getPreRenderBatch();
	renderBatch->reshape(width,height);
}

void PostFX::displaySceneWithAnaglyph(bool deferred){
	_currentCamera->saveCamera();

	_currentCamera->setIOD(_eyeOffset);

	for(I32 i=0; i<2; i++){

		_currentCamera->renderAnaglyph(i == 0 ? true : false);

		_anaglyphFBO[i]->Begin();

			SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);

		_anaglyphFBO[i]->End();
	}

	_currentCamera->restoreCamera();

	_anaglyphShader->bind();
	{
		_anaglyphFBO[0]->Bind(0);
		_anaglyphFBO[1]->Bind(1);

		_anaglyphShader->UniformTexture("texLeftEye", 0);
		_anaglyphShader->UniformTexture("texRightEye", 1);
		_gfx.toggle2D(true);
        _renderQuad->setCustomShader(_anaglyphShader);
		_gfx.renderInstance(_renderQuad->renderInstance());
		_gfx.toggle2D(false);
		_anaglyphFBO[1]->Unbind(1);
		_anaglyphFBO[0]->Unbind(0);
	}
}

void PostFX::displaySceneWithoutAnaglyph(bool deferred){
	_currentCamera->renderLookAt();

	if(_enableDOF){
		_depthFBO->Begin();
			SceneManager::getInstance().render(DOF_STAGE);
		_depthFBO->End();
	}
	_screenFBO->Begin();
		SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
	_screenFBO->End();

	PreRenderStage* renderBatch = PreRenderStageBuilder::getInstance().getPreRenderBatch();
	renderBatch->execute();

	I32 id = 0;
	_postProcessingShader->bind();
		if(!_enableDOF || !_depthOfFieldFBO)	{
			_screenFBO->Bind(id);
		}else{
			_depthOfFieldFBO->Bind(id);
		}

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
			_postProcessingShader->UniformTexture("texBruit", id++);
			_screenBorder->Bind(id);
			_postProcessingShader->UniformTexture("texVignette", id++);
		}
        if(_enableHDR){
            _postProcessingShader->Uniform("exposure",1.0f);
        }
		_postProcessingShader->Uniform("enable_hdr",_enableHDR);

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

		if(!_enableDOF  || !_depthOfFieldFBO)	{
			_screenFBO->Unbind(--id);
		}else{
			_depthOfFieldFBO->Unbind(--id);
		}
}

///Post-Processing in deferred renderer is disabled for now (needs a lot of work)
void PostFX::render(){
	_currentCamera = &(GET_ACTIVE_SCENE()->renderState().getCamera());

	bool deferred = GFX_DEVICE.getRenderer()->getType() != RENDERER_FORWARD;
	if(_enablePostProcessing && !deferred){ /*deferred check here is temporary until PostFx are compatible with deferred rendering -Ionut*/
		_enableAnaglyph ? displaySceneWithAnaglyph(deferred) : 	displaySceneWithoutAnaglyph(deferred);
	}else{
		_currentCamera->renderLookAt();
		SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
	}
}

void PostFX::idle(){
	ParamHandler& par = ParamHandler::getInstance();
	//Update states
	_enablePostProcessing = par.getParam<bool>("postProcessing.enablePostFX");
	_underwater = par.getParam<bool>("scene.camera.underwater");
    _enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
    _enableFXAA = par.getParam<bool>("postProcessing.enableFXAA");

    bool recompileShader = false;

	if(!_postProcessingShader){_enablePostProcessing = false;}

	if(_enablePostProcessing){
		_enableAnaglyph = par.getParam<bool>("postProcessing.enable3D");
        _enableNoise = par.getParam<bool>("postProcessing.enableNoise");

        if(_enableBloom != par.getParam<bool>("postProcessing.enableBloom")){
            _enableBloom = !_enableBloom;
            if(_enableBloom) _postProcessingShader->addShaderDefine("POSTFX_ENABLE_BLOOM");
            else _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_BLOOM");
            recompileShader = true;
        }
        if(_enableSSAO != par.getParam<bool>("postProcessing.enableSSAO")){
            _enableSSAO = !_enableSSAO;
            if(_enableSSAO)  _postProcessingShader->addShaderDefine("POSTFX_ENABLE_SSAO");
            else _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_SSAO");
            recompileShader = true;
        }

        if(_enableHDR != par.getParam<bool>("rendering.enableHDR")){
            _enableHDR = !_enableHDR;
            if(_enableHDR)   _postProcessingShader->addShaderDefine("POSTFX_ENABLE_HDR");
            else _postProcessingShader->removeShaderDefine("POSTFX_ENABLE_HDR");
            recompileShader = true;
        }

        createOperators();
        if(recompileShader){
            ///recreate only the fragment shader
            _postProcessingShader->recompile(false,true);
        }
		_postProcessingShader->bind();
	    if(_enableBloom) _postProcessingShader->Uniform("bloom_factor", par.getParam<F32>("postProcessing.bloomFactor"));
		_postProcessingShader->Uniform("enable_vignette",_enableNoise);
		_postProcessingShader->Uniform("enable_noise",_enableNoise);

		if(_enableNoise){
		    _timer += GETMSTIME();
			if(_timer > _tickInterval ){
				_timer = 0;
				_randomNoiseCoefficient = (F32)random(1000) * 0.001f;
				_randomFlashCoefficient = (F32)random(1000) * 0.001f;
			}

			_postProcessingShader->Uniform("randomCoeffNoise", _randomNoiseCoefficient);
			_postProcessingShader->Uniform("randomCoeffFlash", _randomFlashCoefficient);
		}
		_postProcessingShader->unbind();
	}
}