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
	_blurShader(NULL),
	_SSAOShaderPass1(NULL),
	_noise(NULL),
	_screenBorder(NULL),
	_bloomFBO(NULL),
	_depthOfFieldFBO(NULL),
	_depthFBO(NULL),
	_screenFBO(NULL),
	_SSAO_FBO(NULL),
	_currentCamera(NULL),
	_gfx(GFX_DEVICE){
	_anaglyphFBO[0] = NULL;
	_anaglyphFBO[1] = NULL;
}

PostFX::~PostFX(){
	if(_underwaterTexture){
		RemoveResource(_underwaterTexture);
	}
	SAFE_DELETE(_screenFBO);
	SAFE_DELETE(_depthFBO);
	if(_renderQuad){
		RemoveResource(_renderQuad);
	}
	if(_enablePostProcessing){
		RemoveResource(_postProcessingShader);
		if(_enableAnaglyph){
			RemoveResource(_anaglyphShader);
			SAFE_DELETE(_anaglyphFBO[0]);
			SAFE_DELETE(_anaglyphFBO[1]);
		}
		
		if(_enableDOF || _enableBloom){
			if(_enableBloom){
				SAFE_DELETE(_bloomFBO);
			}
			if(_enableDOF){
				SAFE_DELETE(_depthOfFieldFBO);
			}
			if(_blurShader){
				RemoveResource(_blurShader);
			}
		}
		if(_enableNoise){
			RemoveResource(_noise);
			RemoveResource(_screenBorder);
		}
		if(_enableSSAO){
			SAFE_DELETE(_SSAO_FBO);
			RemoveResource(_SSAOShaderPass1);
		}
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
	_blurShader = NULL;
	_screenFBO = _gfx.newFBO(FBO_2D_COLOR);
	_depthFBO  = _gfx.newFBO(FBO_2D_DEPTH);

	ResourceDescriptor mrt("PostFX RenderQuad");
	mrt.setFlag(true); //No default Material;
	_renderQuad = CreateResource<Quad3D>(mrt);
	assert(_renderQuad);
	_renderQuad->setDimensions(vec4<F32>(0,0,resolution.width,resolution.height));

	if(_enablePostProcessing){
		_postProcessingShader = CreateResource<ShaderProgram>(ResourceDescriptor("postProcessing"));
		if(_enableAnaglyph){
			_anaglyphFBO[0] = _gfx.newFBO(FBO_2D_COLOR);
			_anaglyphFBO[1] = _gfx.newFBO(FBO_2D_COLOR);
			_anaglyphShader = CreateResource<ShaderProgram>(ResourceDescriptor("anaglyph"));
			_eyeOffset = par.getParam<F32>("postProcessing.anaglyphOffset");
		}
		if(_enableBloom){
			_bloomFBO = _gfx.newFBO(FBO_2D_COLOR);
			if(!_blurShader){
				_blurShader = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));
			}
			PreRenderOperator* bloomOp = PreRenderStageBuilder::getInstance().addBloomOperator(_blurShader, _renderQuad,_enableBloom,_bloomFBO,resolution);
			bloomOp->addInputFBO(_screenFBO);
		}
		if(_enableSSAO){
			_SSAO_FBO = _gfx.newFBO(FBO_2D_COLOR);
			_SSAOShaderPass1 = CreateResource<ShaderProgram>(ResourceDescriptor("SSAOPass1"));
			PreRenderOperator* ssaOp = PreRenderStageBuilder::getInstance().addSSAOOperator(_SSAOShaderPass1, _renderQuad,_enableSSAO, _SSAO_FBO,resolution);
		}

		if(_enableDOF){
			_depthOfFieldFBO = _gfx.newFBO(FBO_2D_COLOR);
			if(!_blurShader){
				///DOF uses the same blur shader as Bloom
				_blurShader = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));
			}
			PreRenderOperator* dofOp = PreRenderStageBuilder::getInstance().addDOFOperator(_blurShader, _renderQuad,_enableDOF, _depthOfFieldFBO,resolution);
			dofOp->addInputFBO(_screenFBO);
			dofOp->addInputFBO(_depthFBO);
		}

		if(_enableNoise){
			ResourceDescriptor noiseTexture("noiseTexture");
			ResourceDescriptor borderTexture("borderTexture");
			noiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//bruit_gaussien.jpg");
			borderTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//vignette.jpeg");
			_noise = CreateResource<Texture>(noiseTexture);
			_screenBorder = CreateResource<Texture>(borderTexture);
		}
	}

	ResourceDescriptor textureWaterCaustics("Underwater Caustics");
	textureWaterCaustics.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images/terrain_water_NM.jpg");
	_underwaterTexture = CreateResource<Texture2D>(textureWaterCaustics);

	_timer = 0;
	_tickInterval = 1.0f/24.0f;
	_randomNoiseCoefficient = 0;
	_randomFlashCoefficient = 0;

	par.setParam("postProcessing.enableDepthOfField", false); //enable using keyboard;
	PostFX::getInstance().reshapeFBO(resolution.width, resolution.height);
}

void PostFX::reshapeFBO(I32 width , I32 height){
	
	if((D32)width / (D32)height != 1.3333){
		height = width/1.3333;
	}

	_screenFBO->Create(width, height);
	_depthFBO->Create(width, height);
	_renderQuad->setDimensions(vec4<F32>(0,0,width,height));

	if(!_enablePostProcessing) return;
	if(_enableAnaglyph){
		_anaglyphFBO[0]->Create(width, height);
		_anaglyphFBO[1]->Create(width, height);
	}
	PreRenderStage* renderBatch = PreRenderStageBuilder::getInstance().getPreRenderBatch();
	renderBatch->reshape(width,height);
}

void PostFX::displaySceneWithAnaglyph(void){

	_currentCamera->SaveCamera();
	bool deferred = GFX_DEVICE.getDeferredRendering();
	F32 _eyePos[2] = {_eyeOffset/2, -_eyeOffset};

	for(I32 i=0; i<2; i++){
		_currentCamera->MoveAnaglyph(_eyePos[i]);
		_currentCamera->RenderLookAt();

		_anaglyphFBO[i]->Begin();

			SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
		
		_anaglyphFBO[i]->End();
	}

	_currentCamera->RestoreCamera();

	
	_anaglyphShader->bind();
	{
		_anaglyphFBO[0]->Bind(0);
		_anaglyphFBO[1]->Bind(1);

		_anaglyphShader->UniformTexture("texLeftEye", 0);
		_anaglyphShader->UniformTexture("texRightEye", 1);
		_gfx.toggle2D(true);
		_gfx.renderModel(_renderQuad);
		_gfx.toggle2D(false);
		_anaglyphFBO[1]->Unbind(1);
		_anaglyphFBO[0]->Unbind(0);
	}
	//_anaglyphShader->unbind();

}

void PostFX::displaySceneWithoutAnaglyph(void){

	_currentCamera->RenderLookAt();
	bool deferred = GFX_DEVICE.getDeferredRendering();
	ParamHandler& par = ParamHandler::getInstance();

	if(!_enableDOF){
		_screenFBO->Begin();
			SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
		_screenFBO->End();
	}else{
		_depthFBO->Begin();
			SceneManager::getInstance().render(DEPTH_STAGE);
		_depthFBO->End();
		_screenFBO->Begin();
			SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
		_screenFBO->End();
	}

	PreRenderStage* renderBatch = PreRenderStageBuilder::getInstance().getPreRenderBatch();
	renderBatch->execute();

	I32 id = 0;
	_postProcessingShader->bind();
		if(!_enableDOF)	{
			_screenFBO->Bind(id);
		}else{
			_depthOfFieldFBO->Bind(id);
		}
		
		_postProcessingShader->UniformTexture("texScreen", id++);
		_postProcessingShader->Uniform("enable_bloom",_enableBloom);
		_postProcessingShader->Uniform("enable_ssao",_enableSSAO);
		_postProcessingShader->Uniform("enable_vignette",_enableNoise);
		_postProcessingShader->Uniform("enable_noise",_enableNoise);
		_postProcessingShader->Uniform("enable_pdc",_enableDOF);
		_postProcessingShader->Uniform("enable_underwater",false);	

		if(par.getParam<bool>("underwater")){
			if(_underwaterTexture){
				_underwaterTexture->Bind(id);
				_postProcessingShader->UniformTexture("texWaterNoiseNM", id++);
			}
			_postProcessingShader->Uniform("noise_tile", 0.05f);
			_postProcessingShader->Uniform("noise_factor", 0.02f);
			_postProcessingShader->Uniform("enable_underwater",true);	
		}
			
		if(_enableBloom){
			_bloomFBO->Bind(id);
			_postProcessingShader->UniformTexture("texBloom", id++);
			_postProcessingShader->Uniform("bloom_factor", 0.3f);
		}

		if(_enableSSAO){
			_SSAO_FBO->Bind(id);
			_postProcessingShader->UniformTexture("texSSAO", id++);
		}

		if(_enableNoise){
			_noise->Bind(id);
			_postProcessingShader->UniformTexture("texBruit", id++);

			_postProcessingShader->Uniform("randomCoeffNoise", _randomNoiseCoefficient);
			_postProcessingShader->Uniform("randomCoeffFlash", _randomFlashCoefficient);
			_screenBorder->Bind(id);
			_postProcessingShader->UniformTexture("texVignette", id++);
		}

		_gfx.toggle2D(true);
		_gfx.renderModel(_renderQuad);
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

		if(par.getParam<bool>("underwater") && _underwaterTexture){
			_underwaterTexture->Unbind(--id);
		}

		if(!_enableDOF)	{
			_screenFBO->Unbind(--id);
		}else{
			_depthOfFieldFBO->Unbind(--id);
		}
}
///Post-Processing in deffered renderer is disabled for now (needs a lot of work)
void PostFX::render(Camera* const camera){
	_currentCamera = camera;
	bool deferred = GFX_DEVICE.getDeferredRendering();
	if(_enablePostProcessing && !deferred){
		if(_enableAnaglyph){
			displaySceneWithAnaglyph();
		}else{
			displaySceneWithoutAnaglyph();
		}
	}else{
		_currentCamera->RenderLookAt();
		SceneManager::getInstance().render(deferred ? DEFERRED_STAGE : FINAL_STAGE);
	}

}

void PostFX::idle(){
	ParamHandler& par = ParamHandler::getInstance();
	//Update states
	_enablePostProcessing = par.getParam<bool>("postProcessing.enablePostFX");
	_enableAnaglyph = par.getParam<bool>("postProcessing.enable3D");
	_enableBloom = par.getParam<bool>("postProcessing.enableBloom");
	_enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
	_enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
	_enableNoise = par.getParam<bool>("postProcessing.enableNoise");

	if(_enablePostProcessing){
		if(_enableNoise){
				_timer += GETMSTIME();
				if(_timer > _tickInterval ){
					_timer = 0;
					_randomNoiseCoefficient = (F32)random(1000)/1000.0f;
					_randomFlashCoefficient = (F32)random(1000)/1000.0f;
				}
		}
	}
}
