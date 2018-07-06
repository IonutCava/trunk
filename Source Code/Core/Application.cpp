#include "Headers/Application.h"

#include "GUI/Headers/GUI.h"
#include "Utility/Headers/Kernel.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Hardware/Video/RenderStateBlock.h"

bool Application::_keepAlive = true;

void Application::Idle(){
	SceneManager::getInstance().clean();
	PostFX::getInstance().idle();
	GFX_DEVICE.idle();
	PHYSICS_DEVICE.idle();
	FrameListenerManager::getInstance().idle();
}

//BEGIN CONSTRUCTOR
Application::Application() : 
	_GFX(GFX_DEVICE), //Video
	_SFX(SFXDevice::getInstance()), //Audio
	_scene(SceneManager::getInstance()),
	_gui(GUI::getInstance()),
	_currentTime(0),
	_camera(New FreeFlyCamera()){
	 angleLR=0.0f,angleUD=0.0f,moveFB=0.0f,moveLR=0.0f;
	 mainWindowId = -1;
	 _previewDepthMaps = false;
	 CameraManager::getInstance().addNewCamera("defaultCamera",_camera);
	 ///If camera has been updated
	 _camera->addUpdateListener(boost::bind(&LightManager::update, ///< force all lights to update
										    boost::ref(LightManager::getInstance()),true));
}
//END CONSTRUCTOR


void Application::DrawSceneStatic(){
	/// Update time at every render loop
	Application::getInstance()._currentTime += clock()/ D32( CLOCKS_PER_SEC);
	if(!_keepAlive) {
		Application::getInstance().killApplication(); 
		return;
	}

	GFX_DEVICE.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
	SET_DEFAULT_STATE_BLOCK();
	Framerate::getInstance().SetSpeedFactor();
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_STARTED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameStarted(evt);

	PHYSICS_DEVICE.process();

	_keepAlive = Application::getInstance().DrawScene();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_PROCESS,evt);
	_keepAlive = FrameListenerManager::getInstance().frameRenderingQueued(evt);

	GFX_DEVICE.swapBuffers();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_ENDED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
}


bool Application::DrawScene(){
	_camera->RenderLookAt();	

	_scene.updateSceneState(_currentTime);

	PHYSICS_DEVICE.update();

	_scene.preRender();
	
	/// Inform listeners that we finished pre-rendering
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_PRERENDER_END,evt);
	if(!FrameListenerManager::getInstance().framePreRenderEnded(evt)){
		return false;
	}

	LightManager::getInstance().generateShadowMaps();

	PostFX::getInstance().render();
	ShaderManager::getInstance().unbind();//unbind all shaders
	if(_previewDepthMaps && GFX_DEVICE.getRenderStage() != DEFERRED_STAGE){
		LightManager::getInstance().previewDepthMaps();
	}
	GUI::getInstance().draw();
	_scene.processInput();
	_scene.processEvents(abs(GETTIME()));
	return true;
}

void Application::Initialize(){    
	_GFX.setApi(OpenGL);
	_GFX.initHardware();
	_SFX.initHardware();
	_camera->setEye(vec3<F32>(0,50,0));
	F32 fogColor[4] = {0.2f, 0.2f, 0.4f, 1.0}; 
	_GFX.enableFog(0.01f,fogColor);
	PostFX::getInstance().init();
	PHYSICS_DEVICE.initPhysics();
	_gui.createConsole();

}

void Application::killApplication() {
	_keepAlive = false;
	Application::getInstance().Deinitialize(); ///Destroy platform
}

void Application::Deinitialize(){
	_gui.DestroyInstance(); ///Deactivate GUI
	PRINT_FN("Closing application!");
	PHYSICS_DEVICE.exitPhysics();
	PostFX::getInstance().DestroyInstance();
	Kernel::getInstance().TerminateApplication(); ///Free resources
	PRINT_FN("Closing hardware interface(GFX,SFX,PhysX, input,network) engine ...");
	_SFX.closeAudioApi();
	_SFX.DestroyInstance();
	_GFX.closeRenderingApi();
	_GFX.DestroyInstance();
}