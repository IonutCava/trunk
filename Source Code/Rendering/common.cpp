#include "common.h"

#include "Utility/Headers/Guardian.h"
#include "Managers/ResourceManager.h"
#include "Managers/TerrainManager.h"
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

void Engine::Idle()
{
	SceneManager::getInstance().clean();
	GFXDevice::getInstance().idle();
}

Engine::Engine() : 
	_GFX(GFXDevice::getInstance()), //Video
	_SFX(SFXDevice::getInstance()), //Audio
    _px(PhysX::getInstance()),
	_scene(SceneManager::getInstance()),
	_gui(GUI::getInstance())
{
	//BEGIN CONSTRUCTOR
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 mainWindowId = -1;
	 _camera = new FreeFlyCamera();
	 CameraManager::getInstance().add("defaultCamera",_camera);
	 //END CONSTRUCTOR
}


void Engine::DrawSceneStatic()
{
	GFXDevice::getInstance().clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
	Engine::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Engine::DrawScene()
{

	_GFX.enable_MODELVIEW();
	_GFX.loadIdentityMatrix();
	_camera->RenderLookAt();
	
	if(_px.getScene() != NULL)
	{
		_px.GetPhysicsResults();
		if (_px.getWireFrameData())
			_px.getDebugRenderer()->renderData(*(_px.getScene()->getDebugRenderable()));
		_px.StartPhysics();
	}
	

	std::vector<Light_ptr> & lights = _scene.getActiveScene()->getLights();
	for(U8 i = 0; i < lights.size(); i++)
		lights[i]->update();

	_scene.preRender();
	_scene.render();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
}

void Engine::Initialize()
{    
	ResourceManager& res = ResourceManager::getInstance();
	_GFX.setApi(OpenGL32);
	_GFX.initHardware();
	_SFX.initHardware();
	_camera->setEye(vec3(0,50,0));
	F32 fogColor[4] = {0.7f, 0.7f, 0.9f, 1.0}; 
	_GFX.enableFog(0.3f,fogColor);
}

void Engine::Quit()
{
	Con::getInstance().printfn("Engine shutdown complete...");
	exit(0);
}
