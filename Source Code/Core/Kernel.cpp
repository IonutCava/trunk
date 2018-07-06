#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

#include "Rendering/Headers/ForwardRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"
#include "Rendering/Headers/DeferredLightingRenderer.h"


D32 Kernel::_currentTime = 0.0;
D32 Kernel::_currentTimeMS = 0.0;
bool Kernel::_keepAlive = true;
bool Kernel::_applicationReady = false;
boost::function0<void> Kernel::_mainLoopCallback;

Kernel::Kernel(I32 argc, char **argv) :
					_argc(argc),
					_argv(argv),
                    _loadAI(false),
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
	LightManager::getInstance().idle();
	FrameListenerManager::getInstance().idle();
	std::string pendingLanguage = ParamHandler::getInstance().getParam<std::string>("language");
	if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
		Locale::changeLanguage(pendingLanguage);
	}
}

void Kernel::MainLoopApp(){
	/// Update time at every render loop
	Kernel::_currentTime     = GETTIME();
	Kernel::_currentTimeMS   = GETMSTIME();
	if(!_keepAlive) {
		///exiting the graphical rendering loop will return us to the current control point
		///if we return here, the next valid control point is in main() thus shutding down the application neatly
		GFX_DEVICE.exitRenderLoop(true);
		return;
	}
	Kernel::Idle();
	///Restore CPU to default state: clear buffers and set default state
	GFX_DEVICE.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER | GFXDevice::STENCIL_BUFFER);
	SET_DEFAULT_STATE_BLOCK();
	///Get current frame-to-application speed factor
	Framerate::getInstance().SetSpeedFactor();
	///Launch the FRAME_STARTED event
	FrameEvent evt;
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_STARTED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameStarted(evt);
	///Process physics
	PHYSICS_DEVICE.process();
	///Process the current frame
	_keepAlive = Application::getInstance().getKernel()->MainLoopScene();
	///Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_PROCESS,evt);
	_keepAlive = FrameListenerManager::getInstance().frameRenderingQueued(evt);
	///Swap GPU buffers
	GFX_DEVICE.swapBuffers();
	///Launch the FRAME_ENDED event (buffers have been swapped)
	FrameListenerManager::getInstance().createEvent(FRAME_EVENT_ENDED,evt);
	_keepAlive = FrameListenerManager::getInstance().frameEnded(evt);
}

void Kernel::FirstLoop(){
	///Skip one frame so all resources can be built while the splash screen is displayed
	MainLoopApp();
	///Hide splash screen
	ParamHandler& par = ParamHandler::getInstance();
	vec2<U16> resolution(par.getParam<I32>("runtime.resolutionWidth"),par.getParam<I32>("runtime.resolutionHeight"));
	GFX_DEVICE.setWindowSize(resolution.width,resolution.height);
	GFX_DEVICE.setWindowPos(10,50);
	Kernel::_applicationReady = true; ///<All is set
	//GFX_DEVICE.swapBuffers();
	///Bind main render loop
	_mainLoopCallback = boost::ref(Kernel::MainLoopApp);
}

const int SKIP_TICKS = 1000 / TICKS_PER_SECOND;
bool Kernel::MainLoopScene(){
	///Update camera position
	_camera->RenderLookAt();
	///Set the current scene's active camera
	_sceneMgr.updateCamera(_camera);
	_loops = 0;
//	while(_currentTimeMS > _nextGameTick && _loops < MAX_FRAMESKIP) {
		/// Update scene based on input
		_sceneMgr.processInput();
		/// process all scene events
		_sceneMgr.processEvents(_currentTimeMS);

		///Update the scene state based on current time
		_sceneMgr.updateSceneState(_currentTimeMS);
		///Update physics
		_PFX.update();
		///Prepare scene for rendering
		_sceneMgr.preRender();
	
		/// Inform listeners that we finished pre-rendering
		FrameEvent evt;
		FrameListenerManager::getInstance().createEvent(FRAME_PRERENDER_END,evt);

		if(!FrameListenerManager::getInstance().framePreRenderEnded(evt)){
			return false;
		}

		_nextGameTick += SKIP_TICKS;
        _loops++;
//	}
	presentToScreen();
	/// Get input events
	_inputInterface.tick();
	/// Preview depthmaps if needed
	_lightPool.previewShadowMaps();

	///unbind all shaders
	ShaderManager::getInstance().unbind();

	/// Draw the GUI
	GUI::getInstance().draw();
	return true;
}

void Kernel::presentToScreen(){
	/// When the entire scene is ready for rendering, generate the shadowmaps
	_lightPool.generateShadowMaps(_sceneMgr.getActiveScene()->renderState());
	/// Render the scene adding any post-processing effects that we have active
	PostFX::getInstance().render(_camera);
}


I8 Kernel::Initialize(const std::string& entryPoint) {
	I8 windowId = -1;
	I8 returnCode = 0;
	ParamHandler& par = ParamHandler::getInstance();
	///Using OpenGL for rendering as default
	_GFX.setApi(OpenGL);
	///Target FPS is usually 60. So all movement is capped around that value
	Framerate::getInstance().Init(TARGET_FRAME_RATE);
	///Load info from XML files
	std::string startupScene = XML::loadScripts(entryPoint);
	///Create mem log file
	std::string mem = par.getParam<std::string>("memFile");
	if(mem.compare("none") == 0) mem = "mem.log";
	Application::getInstance().setMemoryLogFile(mem);
	PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
	vec2<U16> resolution(par.getParam<I32>("runtime.resolutionWidth"),par.getParam<I32>("runtime.resolutionHeight"));
	windowId = _GFX.initHardware(resolution/2,_argc,_argv);
	if(windowId < 0){
		///If we could not initialize the graphics device, exit with code -2;
		return windowId;
	}
	///Load the splash screen 
	GUISplash splashScreen("divideLogo.jpg",resolution/2);
	_GFX.setWindowSize(resolution.width/2,resolution.height/2);
	_GFX.clearBuffers(GFXDevice::COLOR_BUFFER | GFXDevice::DEPTH_BUFFER | GFXDevice::STENCIL_BUFFER);
	splashScreen.render();
	_GFX.swapBuffers();
	///We start of with a forward renderer. Change it to something else on scene load ... or ... whenever
	_GFX.setRenderer(New ForwardRenderer());
	_GFX.registerKernel(this);
	_lightPool.init();
	GUI::getInstance().init();
	PRINT_FN(Locale::get("START_SOUND_INTERFACE"));
	returnCode = _SFX.initHardware();
	if(returnCode < 0){
		return returnCode;
	}
	PRINT_FN(Locale::get("START_PHYSICS_INTERFACE"));
	returnCode =_PFX.initPhysics(TARGET_FRAME_RATE);
	if(returnCode < 0) {
		return returnCode;
	}
	PostFX::getInstance().init(resolution);
	///Bind the kernel with the input interface
	_inputInterface.initialize(this,par.getParam<std::string>("appTitle"),(size_t)_GFX.getHWND());
	///Load default material
	PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
	XML::loadMaterialXML(par.getParam<std::string>("scriptLocation")+"/defaultMaterial");
	if(!_sceneMgr.load(startupScene, resolution, _camera)){       ///< Load the scene with a default camera
		ERROR_FN(Locale::get("ERROR_SCENE_LOAD"),startupScene.c_str());
		return MISSING_SCENE_DATA;
	}
	if(!_sceneMgr.checkLoadFlag()){
		ERROR_FN(Locale::get("ERROR_SCENE_LOAD_NOT_CALLED"),startupScene.c_str());
		return MISSING_SCENE_LOAD_CALL;
	}
	PRINT_FN(Locale::get("INITIAL_DATA_LOADED"));
	PRINT_FN(Locale::get("CREATE_AI_ENTITIES_START"));
	///Start the AIManager thread
	_loadAI = _sceneMgr.initializeAI(true);
	PRINT_FN(Locale::get("CREATE_AI_ENTITIES_END"));
	_GUI.cacheResolution(resolution);
	_nextGameTick = GETMSTIME();
	return windowId;
}

void Kernel::beginLogicLoop(){
	PRINT_FN(Locale::get("START_RENDER_LOOP"));
	///Fire up the AI if needed
	if(_loadAI){
		_aiEvent->startEvent();
	}
	///lock the scene
	GET_ACTIVE_SCENE()->state()->toggleRunningState(true);
	ParamHandler& par = ParamHandler::getInstance();
	vec2<U16> resolution(par.getParam<I32>("runtime.resolutionWidth"),par.getParam<I32>("runtime.resolutionHeight"));
	///The first loop compiles all the textures, so, do not render the first frame
	_mainLoopCallback = boost::ref(Kernel::FirstLoop);
	//Target FPS is define in "config.h". So all movement is capped around that value
	//start rendering
	GFX_DEVICE.initDevice(TARGET_FRAME_RATE);
}

void Kernel::Shutdown(){
	_keepAlive = false;
	///release the scene
	GET_ACTIVE_SCENE()->state()->toggleRunningState(false);
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
	while(_aiEvent.get());
	AIManager::getInstance().Destroy();
	AIManager::getInstance().DestroyInstance();
	_sceneMgr.DestroyInstance();
	_GFX.closeRenderer();
	PRINT_FN(Locale::get("STOP_RESOURCE_CACHE"));
	ResourceCache::getInstance().DestroyInstance();
	PRINT_FN(Locale::get("STOP_ENGINE_OK"));

	PRINT_FN(Locale::get("STOP_HARDWARE"));
	_SFX.closeAudioApi();
	_SFX.DestroyInstance();
	_GFX.closeRenderingApi();
	_GFX.DestroyInstance();
	Locale::clear();
}

void Kernel::updateResolutionCallback(I32 w, I32 h){
	if(!_applicationReady) return;
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
	GET_ACTIVE_SCENE()->onKeyDown(key);
	return true;
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
	GET_ACTIVE_SCENE()->onKeyUp(key);
	return true;
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
	GUI::getInstance().checkItem(arg.state.X.abs,arg.state.Y.abs);
	GET_ACTIVE_SCENE()->onMouseMove(arg);
	return true;
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	GET_ACTIVE_SCENE()->onMouseClickDown(arg,button);
	return true;
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	GET_ACTIVE_SCENE()->onMouseClickUp(arg,button);
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