#include "PostFX.h"
#include "Rendering/Application.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/CameraManager.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"

PostFX::PostFX(): _underwaterTexture(NULL),
	_renderQuad(NULL),
	_anaglyphShader(NULL),
	_postProcessingShader(NULL),
	_blurShader(NULL),
	_bloomShader(NULL),
	_noise(NULL),
	_screenBorder(NULL),
	_tempBloomFBO(NULL),
	_bloomFBO(NULL),
	_tempDepthOfFieldFBO(NULL),
	_depthOfFieldFBO(NULL),
	_depthFBO(NULL),
	_screenFBO(NULL),
	_gfx(GFXDevice::getInstance()){
	_anaglyphFBO[0] = NULL;
	_anaglyphFBO[1] = NULL;
}

PostFX::~PostFX(){
	ParamHandler& par = ParamHandler::getInstance();
	if(_underwaterTexture){
		RemoveResource(_underwaterTexture);
	}
	if(_screenFBO){
		delete _screenFBO;
		_screenFBO = NULL;
	}
	if(_depthFBO){
		delete _depthFBO;
		_depthFBO = NULL;
	}
	if(_renderQuad){
		RemoveResource(_renderQuad);
	}
	if(_enablePostProcessing){
		RemoveResource(_postProcessingShader);
		if(_enableAnaglyph){
			RemoveResource(_anaglyphShader);
			if(_anaglyphFBO[0]){
				delete _anaglyphFBO[0];
				_anaglyphFBO[0] = NULL;
			}
			if(_anaglyphFBO[1]){
				delete _anaglyphFBO[1];
				_anaglyphFBO[1] = NULL;
			}
		}
		if(_enableBloom){
			if(_bloomFBO){
				delete _bloomFBO;
				_bloomFBO = NULL;
			}
			if(_tempBloomFBO){
				delete _tempBloomFBO;
				_tempBloomFBO = NULL;
			}
			RemoveResource(_bloomShader);
			RemoveResource(_blurShader);
		}
		if(_enableDOF){
			if(_depthOfFieldFBO){
				delete _depthOfFieldFBO;
				_depthOfFieldFBO = NULL;
			}
			if(_tempDepthOfFieldFBO){
				delete _tempDepthOfFieldFBO;
				_tempDepthOfFieldFBO = NULL;
			}
			if(_blurShader){
				RemoveResource(_blurShader);
			}
		}
		if(_enableNoise){
			RemoveResource(_noise);
			RemoveResource(_screenBorder);
		}

	}
}

void PostFX::init(){
	ParamHandler& par = ParamHandler::getInstance();

	_enablePostProcessing = par.getParam<bool>("enablePostFX");
	_enableAnaglyph = par.getParam<bool>("enable3D");
	_enableBloom = par.getParam<bool>("enableBloom");
	_enableDOF = par.getParam<bool>("enableDepthOfField");
	_enableNoise = par.getParam<bool>("enableNoise");
	_bloomShader = NULL;
	_blurShader = NULL;
	if(_enablePostProcessing){
		_postProcessingShader = ResourceManager::getInstance().loadResource<Shader>(ResourceDescriptor("postProcessing"));
		if(_enableAnaglyph){
			_anaglyphFBO[0] = _gfx.newFBO();
			_anaglyphFBO[1] = _gfx.newFBO();
			_anaglyphShader = ResourceManager::getInstance().loadResource<Shader>(ResourceDescriptor("anaglyph"));
			_eyeOffset = par.getParam<F32>("anaglyphOffset");
		}
		if(_enableBloom){
			_bloomFBO = _gfx.newFBO();
			_tempBloomFBO = _gfx.newFBO();
			_bloomShader = ResourceManager::getInstance().loadResource<Shader>(ResourceDescriptor("bright"));
			_blurShader = ResourceManager::getInstance().loadResource<Shader>(ResourceDescriptor("blur"));
		}
		if(_enableDOF){
			_depthOfFieldFBO = _gfx.newFBO();
			_tempDepthOfFieldFBO = _gfx.newFBO();
			if(!_blurShader){
				_blurShader = ResourceManager::getInstance().loadResource<Shader>(ResourceDescriptor("blur"));
			}
		}
		if(_enableNoise){
			ResourceDescriptor noiseTexture("noieTexture");
			ResourceDescriptor borderTexture("borderTexture");
			noiseTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//bruit_gaussien.jpg");
			borderTexture.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images//vignette.jpeg");
			_noise = ResourceManager::getInstance().loadResource<Texture>(noiseTexture);
			_screenBorder = ResourceManager::getInstance().loadResource<Texture>(borderTexture);
		}
	}
	_screenFBO = _gfx.newFBO();
	_depthFBO  = _gfx.newFBO();

		

	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	ResourceDescriptor mrt("PostFX RenderQuad");
	mrt.setFlag(true); //No default Material;
	_renderQuad = ResourceManager::getInstance().loadResource<Quad3D>(mrt);
	assert(_renderQuad);
	_renderQuad->getCorner(Quad3D::TOP_LEFT) = vec3(0, height, 0);
	_renderQuad->getCorner(Quad3D::TOP_RIGHT) = vec3(width, height, 0);
	_renderQuad->getCorner(Quad3D::BOTTOM_LEFT) = vec3(0,0,0);
	_renderQuad->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(width, 0, 0);
	ResourceDescriptor textureWaterCaustics("Underwater Caustics");
	textureWaterCaustics.setResourceLocation(par.getParam<std::string>("assetsLocation") + "/misc_images/terrain_water_NM.jpg");
	_underwaterTexture = ResourceManager::getInstance().loadResource<Texture2D>(textureWaterCaustics);
	_timer = 0;
	_tickInterval = 1.0f/24.0f;
	_randomNoiseCoefficient = 0;
	_randomFlashCoefficient = 0;
	par.setParam("enableDepthOfField", false); //enable using keyboard;
	PostFX::getInstance().reshapeFBO(Application::getInstance().getWindowDimensions().width, Application::getInstance().getWindowDimensions().height);
}

void PostFX::reshapeFBO(I32 width , I32 height){
	
	ParamHandler& par = ParamHandler::getInstance();

	if((D32)width / (D32)height != 1.33){
		height = width/1.33;
	}


	_screenFBO->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
	_depthFBO->Create(FrameBufferObject::FBO_2D_DEPTH, width, height);
	_renderQuad->getCorner(Quad3D::TOP_LEFT) = vec3(0, height, 0);
	_renderQuad->getCorner(Quad3D::TOP_RIGHT) = vec3(width, height, 0);
	_renderQuad->getCorner(Quad3D::BOTTOM_LEFT) = vec3(0,0,0);
	_renderQuad->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(width, 0, 0);

	if(!_enablePostProcessing) return;
	if(_enableAnaglyph){
		_anaglyphFBO[0]->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
		_anaglyphFBO[1]->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
	}
	if(_enableDOF){
		_tempDepthOfFieldFBO->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
		_depthOfFieldFBO->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
	}
	if(_enableBloom){
		I32 w = width/4, h = height/4;
		_bloomFBO->Create(FrameBufferObject::FBO_2D_COLOR, w, h);
		_tempBloomFBO->Create(FrameBufferObject::FBO_2D_COLOR, w, h);
	}

}

void PostFX::displaySceneWithAnaglyph(void){

	CameraManager::getInstance().getActiveCamera()->SaveCamera();
	F32 _eyePos[2] = {_eyeOffset/2, -_eyeOffset};

	for(I32 i=0; i<2; i++){
		CameraManager::getInstance().getActiveCamera()->MoveAnaglyph(_eyePos[i]);
		CameraManager::getInstance().getActiveCamera()->RenderLookAt();

		_anaglyphFBO[i]->Begin();
		
			_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			SceneManager::getInstance().render();
		
		_anaglyphFBO[i]->End();
	}

	CameraManager::getInstance().getActiveCamera()->RestoreCamera();

	
	_anaglyphShader->bind();
	{
		_anaglyphFBO[0]->Bind(0);
		_anaglyphFBO[1]->Bind(1);

		_anaglyphShader->UniformTexture("texLeftEye", 0);
		_anaglyphShader->UniformTexture("texRightEye", 1);
		_gfx.toggle2D(true);
		_gfx.drawQuad3D(_renderQuad);
		_gfx.toggle2D(false);
		_anaglyphFBO[1]->Unbind(1);
		_anaglyphFBO[0]->Unbind(0);
	}
	_anaglyphShader->unbind();

}

void PostFX::displaySceneWithoutAnaglyph(void){

	CameraManager::getInstance().getActiveCamera()->RenderLookAt();
	ParamHandler& par = ParamHandler::getInstance();

	bool underwater = par.getParam<bool>("underwater");
	_enableDOF = par.getParam<bool>("enableDepthOfField");

	if(!_enableDOF){
		_screenFBO->Begin();
			_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			SceneManager::getInstance().render();
		_screenFBO->End();
	}else{
		_depthFBO->Begin();
			_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			SceneManager::getInstance().render();
		_depthFBO->End();
		_screenFBO->Begin();
			_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
			SceneManager::getInstance().render();
		_screenFBO->End();
	}
	if(_enableBloom){
		generateBloomTexture();
	}

	if(_enableDOF){
		generateDepthOfFieldTexture();
	}

	I32 id = 0;
	_postProcessingShader->bind();
		if(!_enableDOF)	{
			_screenFBO->Bind(id);
		}else{
			_depthOfFieldFBO->Bind(id);
		}
		
		_postProcessingShader->UniformTexture("texScreen", id++);
		_postProcessingShader->Uniform("enable_bloom",_enableBloom);
		_postProcessingShader->Uniform("enable_vignette",_enableNoise);
		_postProcessingShader->Uniform("enable_noise",_enableNoise);
		_postProcessingShader->Uniform("enable_pdc",_enableDOF);
		_postProcessingShader->Uniform("enable_underwater",underwater);	

		if(underwater){
			if(_underwaterTexture){
				_underwaterTexture->Bind(id);
				_postProcessingShader->UniformTexture("texWaterNoiseNM", id++);
			}
			_postProcessingShader->Uniform("screnWidth", Application::getInstance().getWindowDimensions().width);
			_postProcessingShader->Uniform("screnHeight", Application::getInstance().getWindowDimensions().height);
			_postProcessingShader->Uniform("noise_tile", 0.05f);
			_postProcessingShader->Uniform("noise_factor", 0.02f);
			_postProcessingShader->Uniform("time", GETMSTIME());
		}
			
		if(_enableBloom){
			_bloomFBO->Bind(id);
			_postProcessingShader->UniformTexture("texBloom", id++);
			_postProcessingShader->Uniform("bloom_factor", 4.0f);
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
		_gfx.drawQuad3D(_renderQuad);
		_gfx.toggle2D(false);

		if(_enableNoise){
			_screenBorder->Unbind(--id);
			_noise->Unbind(--id);
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
		
	_postProcessingShader->unbind();
	
}



void PostFX::render(){
	if(_enablePostProcessing){
		if(_enableAnaglyph){
			displaySceneWithAnaglyph();
		}else{
			displaySceneWithoutAnaglyph();
		}
	}else{
		CameraManager::getInstance().getActiveCamera()->RenderLookAt();
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
		SceneManager::getInstance().render();
	}

}

void PostFX::idle(){
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



void PostFX::generateBloomTexture(){
	_bloomFBO->Begin();
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);

		_bloomShader->bind();

			_screenFBO->Bind(0);

				_bloomShader->UniformTexture("texScreen", 0);
				_bloomShader->Uniform("threshold", 0.95f);
				_gfx.toggle2D(true);
				_gfx.drawQuad3D(_renderQuad);
				_gfx.toggle2D(false);

			_screenFBO->Unbind(0);

		_bloomShader->unbind();

	_bloomFBO->End();


	//Blur horizontally
	_tempBloomFBO->Begin();
	
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);

		_blurShader->bind();
		
			_bloomFBO->Bind(0);
				_blurShader->UniformTexture("texScreen", 0);
				_blurShader->Uniform("size", vec2((F32)_bloomFBO->getWidth(), (F32)_bloomFBO->getHeight()));
				_blurShader->Uniform("horizontal", true);
				_blurShader->Uniform("kernel_size", 10);
				_gfx.toggle2D(true);
				_gfx.drawQuad3D(_renderQuad);
				_gfx.toggle2D(false);
			_bloomFBO->Unbind(0);
		
		_blurShader->unbind();
		
	_tempBloomFBO->End();


	//Blur vertically
	_bloomFBO->Begin();
	
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);

		_blurShader->bind();
		
			_tempBloomFBO->Bind(0);
				_blurShader->UniformTexture("texScreen", 0);
				_blurShader->Uniform("size", vec2((F32)_tempBloomFBO->getWidth(), (F32)_tempBloomFBO->getHeight()));
				_blurShader->Uniform("horizontal", false);
				_gfx.toggle2D(true);
				_gfx.drawQuad3D(_renderQuad);
				_gfx.toggle2D(false);
			_tempBloomFBO->Unbind(0);
		
		_blurShader->unbind();
		
	_bloomFBO->End();
}

void PostFX::generateDepthOfFieldTexture(){

	_tempDepthOfFieldFBO->Begin();
	
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);

		_blurShader->bind();

			_screenFBO->Bind(0);
			_depthFBO->Bind(1);

			_blurShader->Uniform("screenWidth", Application::getInstance().getWindowDimensions().width);
			_blurShader->Uniform("screenHeight", Application::getInstance().getWindowDimensions().height);
			_blurShader->Uniform("bHorizontal", false);
			_blurShader->UniformTexture("texScreen", 0);
			_blurShader->UniformTexture("texDepth",1);
		
			_gfx.toggle2D(true);
			_gfx.drawQuad3D(_renderQuad);
			_gfx.toggle2D(false);
				
			_depthFBO->Unbind(1);
			_screenFBO->Unbind(0);
		
		_blurShader->unbind();
				
	
	_tempDepthOfFieldFBO->End();

	_depthOfFieldFBO->Begin();
	
		_gfx.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);

		_blurShader->bind();
				
			_tempDepthOfFieldFBO->Bind(0);
			_depthFBO->Bind(1);

			_blurShader->Uniform("screenWidth", Application::getInstance().getWindowDimensions().width);
			_blurShader->Uniform("screenHeight", Application::getInstance().getWindowDimensions().height);
			_blurShader->Uniform("bHorizontal", true);
			_blurShader->UniformTexture("texScreen", 0);
			_blurShader->UniformTexture("texDepth",1);
					
			_gfx.toggle2D(true);
			_gfx.drawQuad3D(_renderQuad);
			_gfx.toggle2D(false);
				
			_depthFBO->Unbind(1);
			_tempDepthOfFieldFBO->Unbind(0);
				
		_blurShader->unbind();
				
	_depthOfFieldFBO->End();
}