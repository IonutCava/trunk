#include "Hardware/Video/OpenGL/glResources.h" //ToDo: Remove this from here -Ionut

#include "common.h"

#include "GUI/GLUIManager.h"
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

void Engine::Idle()
{
	SceneManager::getInstance().clean();
	GFXDevice::getInstance().idle();
}

Engine::Engine() : 
	_GFX(GFXDevice::getInstance()),
    _px(PhysX::getInstance()),
	_scene(SceneManager::getInstance()),
	_gui(GUI::getInstance())
{
	//BEGIN CONSTRUCTOR
	 time = 0;
	 timebase = 0;
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 mainWindowId = -1;
	 _camera = new FreeFlyCamera();
	 CameraManager::getInstance().add("defaultCamera",_camera);
	 //END CONSTRUCTOR
}


void Engine::DrawSceneStatic()
{
	GFXDevice::getInstance().clearBuffers(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	Engine::getInstance().DrawScene();
	Framerate::getInstance().SetSpeedFactor();
	GFXDevice::getInstance().swapBuffers();
}

void Engine::DrawScene()
{

	RefreshMetrics();
	
	_GFX.enable_MODELVIEW();
	_GFX.loadIdentityMatrix();
	_camera->RenderLookAt();
	_px.GetPhysicsResults();
	if(_px.getScene() != NULL)
	{
		if (_px.getWireFrameData())
			_px.getDebugRenderer()->renderData(*(_px.getScene()->getDebugRenderable()));
	}
	_px.StartPhysics();

	std::vector<Light*> & lights = _scene.getActiveScene()->getLights();
	for(U8 i = 0; i < lights.size(); i++)
		lights[i]->update();

	_scene.preRender();
	_scene.render();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
}

void Engine::RefreshMetrics()
{
	time=(int)(GETTIME()*1000);
	if (time - timebase > 1000) timebase = time;		
}

void Engine::Initialize()
{    
	ResourceManager& res = ResourceManager::getInstance();
	_GFX.setApi(OpenGL32);
	_GFX.initHardware();
	_camera->setEye(vec3(0,50,0));
	F32 fogColor[4] = {0.7f, 0.7f, 0.9f, 1.0}; 
	_GFX.enableFog(0.3f,fogColor);
}

void Engine::Quit()
{
	Con::getInstance().printfn("Destroying Terrain ...");
	SceneManager::getInstance().getTerrainManager()->destroy();
	Con::getInstance().printfn("Closing the rendering engine ...");
	_GFX.closeRenderingApi();
}
