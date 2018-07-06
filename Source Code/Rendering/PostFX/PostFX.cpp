#include "PostFX.h"
#include "Rendering/common.h"
#include "Hardware/Video/FrameBufferObject.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/CameraManager.h"
#include "Managers/SceneManager.h"
#include "Managers/ResourceManager.h"

PostFX::~PostFX(){
	ParamHandler& par = ParamHandler::getInstance();
	ResourceManager::getInstance().remove(_renderQuad);
	if(_screenFBO){
		delete _screenFBO;
		_screenFBO = NULL;
	}
	if(_depthFBO){
		delete _depthFBO;
		_depthFBO = NULL;
	}
	
	if(_enablePostProcessing){
		ResourceManager::getInstance().remove(_postProcessingShader);
		if(_enableAnaglyph){
			ResourceManager::getInstance().remove(_anaglyphShader);
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

	if(_enablePostProcessing){
		_postProcessingShader = ResourceManager::getInstance().LoadResource<Shader>(ResourceDescriptor("postProcessing"));
		if(_enableAnaglyph){
			_anaglyphFBO[0] = GFXDevice::getInstance().newFBO();
			_anaglyphFBO[1] = GFXDevice::getInstance().newFBO();
			_anaglyphShader = ResourceManager::getInstance().LoadResource<Shader>(ResourceDescriptor("anaglyph"));
			_eyeOffset = par.getParam<F32>("anaglyphOffset");
		}
		if(_enableBloom){
			_bloomFBO = GFXDevice::getInstance().newFBO();
			_tempBloomFBO = GFXDevice::getInstance().newFBO();
		}
		if(_enableDOF){
			_depthOfFieldFBO = GFXDevice::getInstance().newFBO();
			_tempDepthOfFieldFBO = GFXDevice::getInstance().newFBO();
		}
	}
	_screenFBO = GFXDevice::getInstance().newFBO();
	 _depthFBO = GFXDevice::getInstance().newFBO();

		

	F32 width = Engine::getInstance().getWindowDimensions().width;
	F32 height = Engine::getInstance().getWindowDimensions().height;
	ResourceDescriptor mrt("PostFX RenderQuad");
	_renderQuad = ResourceManager::getInstance().LoadResource<Quad3D>(mrt);
	assert(_renderQuad != NULL);
	_renderQuad->getCorner(Quad3D::TOP_LEFT) = vec3(0, height, 0);
	_renderQuad->getCorner(Quad3D::TOP_RIGHT) = vec3(width, height, 0);
	_renderQuad->getCorner(Quad3D::BOTTOM_LEFT) = vec3(0,0,0);
	_renderQuad->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(width, 0, 0);
	_renderQuad->setMaterial(NULL);
	_renderQuad->useDefaultMaterial(false);
	_renderQuad->setTransform(NULL);
	_renderQuad->useDefaultTransform(false);

	_timer = 0;
	_tickInterval = 1.0f/24.0f;
	_randomNoiseCoefficient = 0;
	_randomFlashCoefficient = 0;
}

void PostFX::reshapeFBO(I32 width , I32 height){
	
	ParamHandler& par = ParamHandler::getInstance();
	_screenFBO->Create(FrameBufferObject::FBO_2D_COLOR, width, height);
	_depthFBO->Create(FrameBufferObject::FBO_2D_DEPTH, width, height);
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
		GFXDevice::getInstance().enable_MODELVIEW();
		GFXDevice::getInstance().loadIdentityMatrix();
		CameraManager::getInstance().getActiveCamera()->MoveAnaglyph(_eyePos[i]);
		CameraManager::getInstance().getActiveCamera()->RenderLookAt();

		_anaglyphFBO[i]->Begin();
		
			GFXDevice::getInstance().clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
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
		GFXDevice::getInstance().toggle2D(true);
		GFXDevice::getInstance().drawQuad3D(_renderQuad);
		GFXDevice::getInstance().toggle2D(false);
		_anaglyphFBO[1]->Unbind(1);
		_anaglyphFBO[0]->Unbind(0);
	}
	_anaglyphShader->unbind();

}

void PostFX::displaySceneWithoutAnaglyph(void){

	if(_enablePostProcessing){
	}
	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().loadIdentityMatrix();
	CameraManager::getInstance().getActiveCamera()->RenderLookAt();
	
}



void PostFX::render(){
	if(_enablePostProcessing){
		if(_enableAnaglyph){
			displaySceneWithAnaglyph();
		}else{
			displaySceneWithoutAnaglyph();
		}
	}else{
		GFXDevice::getInstance().enable_MODELVIEW();
		GFXDevice::getInstance().loadIdentityMatrix();
		CameraManager::getInstance().getActiveCamera()->RenderLookAt();
		SceneManager::getInstance().render();
	}

}

void PostFX::idle(){
	if(_enablePostProcessing){
		if(_enableNoise){
				_timer += GETMSTIME();
				if(_timer > _tickInterval ){
					_timer = 0;
					_randomNoiseCoefficient = (float)random(1000)/1000.0f;
				}
		}
	}
}




