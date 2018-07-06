#include "Application.h"

#include "Utility/Headers/Guardian.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Terrain/Sky.h"
#include "Managers/CameraManager.h"
#include "Camera/FreeFlyCamera.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Frustum.h"
#include "Utility/Headers/ParamHandler.h"
#include "GUI/GUI.h"
#include "Framerate.h"
#include "Utility/Headers/Event.h"
#include "Geometry/Predefined/Text3D.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Rendering/PostFX/PostFX.h"

void Application::Idle(){
	SceneManager::getInstance().clean();
	PostFX::getInstance().idle();
	GFXDevice::getInstance().idle();
}

Application::Application() : 
	_GFX(GFXDevice::getInstance()), //Video
	_SFX(SFXDevice::getInstance()), //Audio
    _px(PhysX::getInstance()),
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
	Application::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Application::DrawScene(){
	_camera->RenderLookAt();	
	for_each(Light* light, _scene.getLights()){
		light->onDraw();
	}

	if(_px.getScene() != NULL){
		_px.GetPhysicsResults();
		if (_px.getWireFrameData())
			_px.getDebugRenderer()->renderData(*(_px.getScene()->getDebugRenderable()));
		_px.StartPhysics();
	}
	
	_scene.preRender();
	PostFX::getInstance().render();
	GUI::getInstance().draw();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
}

void Application::Initialize(){    
	ResourceManager& res = ResourceManager::getInstance();
	_GFX.setApi(OpenGL32);
	_GFX.initHardware();
	_SFX.initHardware();
	_camera->setEye(vec3(0,50,0));
	F32 fogColor[4] = {0.7f, 0.7f, 0.9f, 1.0}; 
	_GFX.enableFog(0.3f,fogColor);
	PostFX::getInstance().init();
}
