#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Input/Headers/InputInterface.h"

D32 Kernel::_currentTime = 0.0;
bool Kernel::_keepAlive = true;

Kernel::Kernel() :	_loadAI(false),
					_targetFrameRate(60),
					_GFX(GFX_DEVICE),               ///Video
				    _SFX(SFXDevice::getInstance()), ///Audio
					_PFX(PHYSICS_DEVICE),           ///Physics
					_GUI(GUI::getInstance()),       ///Graphical User Interface
				    _sceneMgr(SceneManager::getInstance()), ///Scene Manager 
					_camera(New FreeFlyCamera()),			///Default camera
					_cameraMgr(New CameraManager()),        ///Camera manager
					_lightPool(LightManager::getInstance()),///Light pool
					_inputInterface(InputInterface::getInstance())  ///Input interface
{

	 assert(_cameraMgr != NULL);
	 ///If camera has been updated, set a callback to inform the current scene
	 _cameraMgr->addCameraChangeListener(boost::bind(&SceneManager::updateCamera, ///update camera
													 boost::ref(_sceneMgr),
													 _cameraMgr->getActiveCamera()));
	 _camera->setEye(vec3<F32>(0,50,0));
	 /// force all lights to update
	 _camera->addUpdateListener(boost::bind(&LightManager::update, boost::ref(_lightPool), true));
	 ///As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
	 _cameraMgr->addNewCamera("defaultCamera",_camera);
	 ///Create an AI thread, but start it only if needed
	 _aiEvent.reset(New Event(10,false,false,boost::bind(&AIManager::tick, boost::ref(AIManager::getInstance()))));
}

Kernel::~Kernel(){
	SAFE_DELETE(_cameraMgr);
	_inputInterface.terminate();
	_inputInterface.DestroyInstance();
}

void Kernel::Idle(){
	SceneManager::getInstance().idle();
	PostFX::getInstance().idle();
	GFX_DEVICE.idle();
	PHYSICS_DEVICE.idle();
	FrameListenerManager::getInstance().idle();
	std::string pendingLanguage = ParamHandler::getInstance().getParam<std::string>("language");
	if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
		Locale::changeLanguage(pendingLanguage);
	}
}

void Kernel::MainLoopStatic(){
	/// Update time at every render loop
	Kernel::_currentTime += clock()/ D32( CLOCKS_PER_SEC);
	if(!_keepAlive) {
		///exiting the graphical rendering loop will return us to the current control point
		///if we return here, the next valid control point is in main() thus shutding down the application neatly
		GFX_DEVICE.exitRenderLoop(true);
		return;
	}

	GFX_DEVICE.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER);
	SET_DEFAULT_STATE_BLOCK();
	Framerate::getInstance().SetSpeedFactor();
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_STARTED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameStarted(evt);

	PHYSICS_DEVICE.process();

	_keepAlive = Application::getInstance().getKernel()->MainLoop();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_PROCESS,evt);
	_keepAlive = FrameListenerManager::getInstance().frameRenderingQueued(evt);

	GFX_DEVICE.swapBuffers();

	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_ENDED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
}

bool Kernel::MainLoop(){

	_camera->RenderLookAt();	
	_sceneMgr.updateCamera(_camera);

	_sceneMgr.updateSceneState(_currentTime);

	_PFX.update();

	_sceneMgr.preRender();
	
	/// Inform listeners that we finished pre-rendering
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_PRERENDER_END,evt);

	if(!FrameListenerManager::getInstance().framePreRenderEnded(evt)){
		return false;
	}

	_lightPool.generateShadowMaps(_camera->getEye());

	PostFX::getInstance().render(_camera);

	///unbind all shaders
	ShaderManager::getInstance().unbind();

	///Preview depthmaps if needed
	_lightPool.previewDepthMaps();

	GUI::getInstance().draw();
	_inputInterface.tick();
	_sceneMgr.processInput();
	_sceneMgr.processEvents(abs(GETTIME()));
	return true;
}

I8 Kernel::Initialize(const std::string& entryPoint) {
	I8 windowId = -1;
	I8 returnCode = 0;
	ParamHandler& par = ParamHandler::getInstance();
	///Using OpenGL for rendering as default
	_GFX.setApi(OpenGL);
	///Target FPS is usually 60. So all movement is capped around that value
	Framerate::getInstance().Init(_targetFrameRate);
	///Load info from XML files
	std::string startupScene = XML::loadScripts(entryPoint);
	///Create mem log file
	std::string mem = par.getParam<std::string>("memFile");
	if(mem.compare("none") == 0) mem = "mem.log";
	Application::getInstance().setMemoryLogFile(mem);
	PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
	vec2<U16> resolution(par.getParam<I32>("resolutionWidth"),par.getParam<I32>("resolutionHeight"));
	windowId = _GFX.initHardware(resolution);
	if(windowId < 0){
		///If we could not initialize the graphics device, exit with code -2;
		return windowId;
	}
	_GFX.registerKernel(this);
	PRINT_FN(Locale::get("START_SOUND_INTERFACE"));
	returnCode = _SFX.initHardware();
	if(returnCode < 0){
		return returnCode;
	}
	PRINT_FN(Locale::get("START_PHYSICS_INTERFACE"));
	returnCode =_PFX.initPhysics();
	if(returnCode < 0) {
		return returnCode;
	}
	PostFX::getInstance().init(resolution);
	///Bind the kernel with the input interface
	_inputInterface.initialize(this,par.getParam<std::string>("appTitle"));
	///Load default material
	PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
	XML::loadMaterialXML(par.getParam<std::string>("scriptLocation")+"/defaultMaterial");
	if(!_sceneMgr.load(startupScene, resolution, _camera)){       ///< Load the scene with a default camera
		ERROR_FN(Locale::get("ERROR_SCENE_LOAD"),startupScene.c_str());
		return MISSING_SCENE_DATA;
	}
	PRINT_FN(Locale::get("INITIAL_DATA_LOADED"));
	PRINT_FN(Locale::get("CREATE_AI_ENTITIES_START"));
	///Start the AIManager thread
	_loadAI = _sceneMgr.initializeAI(true);
	PRINT_FN(Locale::get("CREATE_AI_ENTITIES_END"));
	_GUI.cacheResolution(resolution);
	_GUI.createConsole();
	return windowId;
}

void Kernel::beginLogicLoop(){
	PRINT_FN(Locale::get("START_RENDER_LOOP"));
	if(_loadAI){
		_aiEvent->startEvent();
	}
	//Target FPS is 60. So all movement is capped around that value
	GFX_DEVICE.initDevice(_targetFrameRate);
}

void Kernel::Shutdown(){
	_keepAlive = false;
	PRINT_FN(Locale::get("STOP_GUI"));
	_GUI.DestroyInstance(); ///Deactivate GUI
	PRINT_FN(Locale::get("STOP_PHYSICS_INTERFACE"));
	_PFX.exitPhysics();
	PRINT_FN(Locale::get("STOP_POST_FX"));
	PostFX::getInstance().DestroyInstance();
	PRINT_FN(Locale::get("STOP_SCENE_MANAGER"));
	_sceneMgr.deinitializeAI(true);
	///Shut down AIManager thread
	if(_aiEvent.get()){
		_aiEvent->stopEvent();
		_aiEvent.reset();
	}
	AIManager::getInstance().Destroy();
	AIManager::getInstance().DestroyInstance();
	_sceneMgr.DestroyInstance();
	PRINT_FN(Locale::get("STOP_RESOURCE_CACHE"));
	ResourceCache::getInstance().DestroyInstance();
	PRINT_FN(Locale::get("STOP_ENGINE_OK"));

	PRINT_FN(Locale::get("STOP_HARDWARE"));
	_SFX.closeAudioApi();
	_SFX.DestroyInstance();
	_GFX.closeRenderingApi();
	_GFX.DestroyInstance();
}

void Kernel::updateResolutionCallback(I32 w, I32 h){
	///Update rendering device
	GFX_DEVICE.changeResolution(w,h);
	Application::getInstance().setResolutionWidth(w);
	Application::getInstance().setResolutionHeight(h);
	///Update post-processing render targets and buffers
	PostFX::getInstance().reshapeFBO(w, h);
	vec2<U16> newResolution(w,h);
	///Update the graphical user interface
	GUI::getInstance().onResize(newResolution);
	/// Cache resolution for faster access
	SceneManager::getInstance().cacheResolution(newResolution);
}

///--------------------------Input Management-------------------------------------///
///The console is a global GUI element, scene agnostic, so it's handled in the Kernel


bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onKeyDown(key);
	}else{
		GET_ACTIVE_SCENE()->onKeyDown(key);
	}
	return true;
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onKeyUp(key);
	}else{
		GET_ACTIVE_SCENE()->onKeyUp(key);
	}
	return true;
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseMove(arg);
	}else{
		GUI::getInstance().checkItem(arg.state.X.abs,arg.state.Y.abs);
		GET_ACTIVE_SCENE()->onMouseMove(arg);
	}
	return true;
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseClickDown(arg,button);
	}else{
		GET_ACTIVE_SCENE()->onMouseClickDown(arg,button);
	}
	return true;
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUIConsole::getInstance().isConsoleOpen()){
		GUIConsole::getInstance().onMouseClickUp(arg,button);
	}else{
		GET_ACTIVE_SCENE()->onMouseClickUp(arg,button);
	}
	return true;
}

bool Kernel::OnJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis) {
	GET_ACTIVE_SCENE()->OnJoystickMoveAxis(arg,axis);
	return true;
}

bool Kernel::OnJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
	GET_ACTIVE_SCENE()->OnJoystickMovePOV(arg,pov);
	return true;
}

bool Kernel::OnJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
	GET_ACTIVE_SCENE()->OnJoystickButtonDown(arg,button);
	return true;
}

bool Kernel::OnJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
	GET_ACTIVE_SCENE()->OnJoystickButtonUp(arg,button);
	return true;
}