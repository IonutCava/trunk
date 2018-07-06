#include "Headers/Application.h"

#include "GUI/Headers/GUI.h"
#include "Utility/Headers/Guardian.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Headers/FrameListener.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"

bool Application::_keepAlive = true;

void Application::Idle(){
	SceneManager::getInstance().clean();
	PostFX::getInstance().idle();
	GFXDevice::getInstance().idle();
	PXDevice::getInstance().idle();
	FrameListenerManager::getInstance().idle();
}

//BEGIN CONSTRUCTOR
Application::Application() : 
	_GFX(GFXDevice::getInstance()), //Video
	_SFX(SFXDevice::getInstance()), //Audio
	_scene(SceneManager::getInstance()),
	_gui(GUI::getInstance()),
	_camera(New FreeFlyCamera()){
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 mainWindowId = -1;
	 _previewDepthMaps = false;
	 CameraManager::getInstance().add("defaultCamera",_camera);
}
//END CONSTRUCTOR


void Application::DrawSceneStatic(){
	if(_keepAlive){
		FrameEvent evt;
		FrameListenerManager::getInstance().createEvent(FRAME_EVENT_STARTED,evt);
		_keepAlive = FrameListenerManager::getInstance().frameStarted(evt);
		GFXDevice::getInstance().clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
		PXDevice::getInstance().process();
		Application::getInstance().DrawScene();
		Framerate::getInstance().SetSpeedFactor();
		FrameListenerManager::getInstance().createEvent(FRAME_EVENT_PROCESS,evt);
		_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
		GFXDevice::getInstance().swapBuffers();
		FrameListenerManager::getInstance().createEvent(FRAME_EVENT_ENDED,evt);
		_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
	}else{
		Guardian::getInstance().TerminateApplication();
	}
}

void Application::DrawScene(){
	_camera->RenderLookAt();	
	LightManager::getInstance().update();
	PXDevice::getInstance().update();
	_scene.preRender();
	LightManager::getInstance().generateShadowMaps();
	PostFX::getInstance().render();
	ShaderManager::getInstance().unbind();//unbind all shaders
	if(_previewDepthMaps){
		LightManager::getInstance().previewDepthMaps();
	}
	GUI::getInstance().draw();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
}

void Application::Initialize(){    
	_GFX.setApi(OpenGL32);
	_GFX.initHardware();
	_SFX.initHardware();
	_camera->setEye(vec3(0,50,0));
	F32 fogColor[4] = {0.2f, 0.2f, 0.4f, 1.0}; 
	_GFX.enableFog(0.01f,fogColor);
	PostFX::getInstance().init();
	PXDevice::getInstance().initPhysics();
}
