#include "Headers/Application.h"

#include "GUI/Headers/GUI.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/LightManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Audio/SFXDevice.h"
#include "PhysX/Headers/PhysX.h"

void Application::Idle(){
	SceneManager::getInstance().clean();
	PostFX::getInstance().idle();
	GFXDevice::getInstance().idle();
	PhysX::getInstance().idle();
}

Application::Application() : 
	_GFX(GFXDevice::getInstance()), //Video
	_SFX(SFXDevice::getInstance()), //Audio
	_scene(SceneManager::getInstance()),
	_gui(GUI::getInstance()),
	_camera(New FreeFlyCamera()){
	//BEGIN CONSTRUCTOR
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 mainWindowId = -1;
	 
	 CameraManager::getInstance().add("defaultCamera",_camera);
	 //END CONSTRUCTOR
}


void Application::DrawSceneStatic(){
	GFXDevice::getInstance().clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
	PhysX::getInstance().process();
	Application::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Application::DrawScene(){
	LightManager::getInstance().update();
	_camera->RenderLookAt();	
	PhysX::getInstance().update();
	_scene.preRender();
	LightManager::getInstance().generateShadowMaps();
	PostFX::getInstance().render();
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
	PhysX::getInstance().initNx();
}
