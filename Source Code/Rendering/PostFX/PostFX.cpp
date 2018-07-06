#include "Headers/PostFX.h"
#include "Headers/PreRenderStageBuilder.h"
#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"
#include "Core/Headers/Application.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Hardware/Video/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

PostFX::PostFX(): _underwaterTexture(NULL),
	_renderQuad(NULL),
	_anaglyphShader(NULL),
	_postProcessingShader(NULL),
	_blurShader(NULL),
	_SSAOShaderPass1(NULL),
	_noise(NULL),
	_screenBorder(NULL),
	_bloomFBO(NULL),
	_tempDepthOfFieldFBO(NULL),
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
		if(_enableBloom){
			SAFE_DELETE(_bloomFBO);
			RemoveResource(_blurShader);
		}
		if(_enableDOF){
			SAFE_DELETE(_depthOfFieldFBO);
			SAFE_DELETE(_tempDepthOfFieldFBO);
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

void PostFX::init(){
	ParamHandler& par = ParamHandler::getInstance();

	_enablePostProcessing = par.getParam<bool>("postProcessing.enablePostFX");
	_enableAnaglyph = par.getParam<bool>("postProcessing.enable3D");
	_enableBloom = par.getParam<bool>("postProcessing.enableBloom");
	_enableSSAO = par.getParam<bool>("postProcessing.enableSSAO");
	_enableDOF = par.getParam<bool>("postProcessing.enableDepthOfField");
	_enableNoise = par.getParam<bool>("postProcessing.enableNoise");
	_blurShader = NULL;
	_screenFBO = _gfx.newFBO();
	_depthFBO  = _gfx.newFBO();

	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	ResourceDescriptor mrt("PostFX RenderQuad");
	mrt.setFlag(true); //No default Material;
	_renderQuad = CreateResource<Quad3D>(mrt);
	assert(_renderQuad);
	_renderQuad->setDimensions(vec4<F32>(0,0,width,height));

	if(_enablePostProcessing){
		_postProcessingShader = CreateResource<ShaderProgram>(ResourceDescriptor("postProcessing"));
		if(_enableAnaglyph){
			_anaglyphFBO[0] = _gfx.newFBO();
			_anaglyphFBO[1] = _gfx.newFBO();
			_anaglyphShader = CreateResource<ShaderProgram>(ResourceDescriptor("anaglyph"));
			_eyeOffset = par.getParam<F32>("postProcessing.anaglyphOffset");
		}
		if(_enableBloom){
			_bloomFBO = _gfx.newFBO();
			if(!_blurShader){
				_blurShader = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));
			}
			PreRenderOperator* bloomOp = PreRenderStageBuilder::getInstance().addBloomOperator(_blurShader, _renderQuad,_enableBloom,_bloomFBO);
			bloomOp->addInputFBO(_screenFBO);
		}
		if(_enableSSAO){
			_SSAO_FBO = _gfx.newFBO();
			_SSAOShaderPass1 = CreateResource<ShaderProgram>(ResourceDescriptor("SSAOPass1"));
			PreRenderOperator* ssaOp = PreRenderStageBuilder::getInstance().addSSAOOperator(_SSAOShaderPass1, _renderQuad,_enableSSAO, _SSAO_FBO);
		}

		if(_enableDOF){
			_depthOfFieldFBO = _gfx.newFBO();
			_tempDepthOfFieldFBO = _gfx.newFBO();
			if(!_blurShader){
				_blurShader = CreateResource<ShaderProgram>(ResourceDescriptor("blur"));
			}
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
	PostFX::getInstance().reshapeFBO(Application::getInstance().getWindowDimensions().width, Application::getInstance().getWindowDimensions().height);
}

void PostFX::reshapeFBO(I32 width , I32 height){
	
	if((D32)width / (D32)height != 1.3333){
		height = width/1.3333;
	}

	_screenFBO->Create(FBO_2D_COLOR, width, height);
	_depthFBO->Create(FBO_2D_DEPTH, width, height);
	_renderQuad->setDimensions(vec4<F32>(0,0,width,height));

	if(!_enablePostProcessing) return;
	if(_enableAnaglyph){
		_anaglyphFBO[0]->Create(FBO_2D_COLOR, width, height);
		_anaglyphFBO[1]->Create(FBO_2D_COLOR, width, height);
	}
	if(_enableDOF){
		_tempDepthOfFieldFBO->Create(FBO_2D_COLOR, width, height);
		_depthOfFieldFBO->Create(FBO_2D_COLOR, width, height);
	}
	if(_enableSSAO){
		_SSAO_FBO->Create(FBO_2D_COLOR, width, height);
	}
	if(_enableBloom){
		_bloomFBO->Create(FBO_2D_COLOR, width/4, height/4);
	}

	PreRenderStage* renderBatch = PreRenderStageBuilder::getInstance().getPreRenderBatch();
	renderBatch->reshape(width,height);
}

void PostFX::displaySceneWithAnaglyph(void){

	_currentCamera->SaveCamera();
	F32 _eyePos[2] = {_eyeOffset/2, -_eyeOffset};

	for(I32 i=0; i<2; i++){
		_currentCamera->MoveAnaglyph(_eyePos[i]);
		_currentCamera->RenderLookAt();

		_anaglyphFBO[i]->Begin();

			SceneManager::getInstance().render(FINAL_STAGE);
		
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
	ParamHandler& par = ParamHandler::getInstance();

	bool underwater = par.getParam<bool>("underwater");

	if(!_enableDOF){
		_screenFBO->Begin();
			SceneManager::getInstance().render(FINAL_STAGE);
		_screenFBO->End();
	}else{
		_depthFBO->Begin();
			SceneManager::getInstance().render(FINAL_STAGE);
		_depthFBO->End();
		_screenFBO->Begin();
			SceneManager::getInstance().render(FINAL_STAGE);
		_screenFBO->End();
	}
	if(_enableDOF){
		generateDepthOfFieldTexture();
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
		_postProcessingShader->Uniform("enable_underwater",underwater);	

		if(underwater){
			if(_underwaterTexture){
				_underwaterTexture->Bind(id);
				_postProcessingShader->UniformTexture("texWaterNoiseNM", id++);
			}
			_postProcessingShader->Uniform("noise_tile", 0.05f);
			_postProcessingShader->Uniform("noise_factor", 0.02f);
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

		if(underwater && _underwaterTexture){
			_underwaterTexture->Unbind(--id);
		}

		if(!_enableDOF)	{
			_screenFBO->Unbind(--id);
		}else{
			_depthOfFieldFBO->Unbind(--id);
		}
		
	//_postProcessingShader->unbind();
	
}



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

void PostFX::generateDepthOfFieldTexture(){

	_tempDepthOfFieldFBO->Begin();
	
		_blurShader->bind();

			_screenFBO->Bind(0);
			_depthFBO->Bind(1);
			_blurShader->Uniform("bHorizontal", false);
			_blurShader->UniformTexture("texScreen", 0);
			_blurShader->UniformTexture("texDepth",1);
		
			_gfx.toggle2D(true);
			_gfx.renderModel(_renderQuad);
			_gfx.toggle2D(false);
				
			_depthFBO->Unbind(1);
			_screenFBO->Unbind(0);
		
		//_blurShader->unbind();
				
	
	_tempDepthOfFieldFBO->End();

	_depthOfFieldFBO->Begin();

		_blurShader->bind();
				
			_tempDepthOfFieldFBO->Bind(0);
			_depthFBO->Bind(1);

			_blurShader->Uniform("bHorizontal", true);
			_blurShader->UniformTexture("texScreen", 0);
			_blurShader->UniformTexture("texDepth",1);
					
			_gfx.toggle2D(true);
			_gfx.renderModel(_renderQuad);
			_gfx.toggle2D(false);
				
			_depthFBO->Unbind(1);
			_tempDepthOfFieldFBO->Unbind(0);
				
		//_blurShader->unbind();
				
	_depthOfFieldFBO->End();
}