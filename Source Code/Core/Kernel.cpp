#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
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

U32 Kernel::_currentTime = 0;
U32 Kernel::_currentTimeMS = 0;
bool Kernel::_keepAlive = true;
bool Kernel::_applicationReady = false;
bool Kernel::_renderingPaused = false;

boost::function0<void> Kernel::_mainLoopCallback;

Kernel::Kernel(I32 argc, char **argv, Application& parentApp) :
					_argc(argc),
					_argv(argv),
					_APP(parentApp),
					_GFX(GFX_DEVICE),               //Video
				    _SFX(SFXDevice::getInstance()), //Audio
					_PFX(PHYSICS_DEVICE),           //Physics
					_GUI(GUI::getInstance()),       //Graphical User Interface
				    _sceneMgr(SceneManager::getInstance()),  //Scene Manager
                    _shaderMgr(ShaderManager::getInstance()),//Shader Manager
					_camera(New FreeFlyCamera()),			 //Default camera
					_cameraMgr(New CameraManager()),         //Camera manager
					_frameMgr(FrameListenerManager::getInstance()),
					_lightPool(LightManager::getInstance()),//Light pool
					_inputInterface(InputInterface::getInstance())  //Input interface
{
	 assert(_cameraMgr != NULL);
	 //If camera has been updated, set a callback to inform the current scene
	 _cameraMgr->addCameraChangeListener(DELEGATE_BIND(&SceneManager::updateCamera, //update camera
													 DELEGATE_REF(_sceneMgr),
													 _cameraMgr->getActiveCamera()));
     // force all lights to update on camera change (to keep them still actually)
	 _camera->addUpdateListener(DELEGATE_BIND(&LightManager::update, DELEGATE_REF(_lightPool)));
	 //As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
	 _cameraMgr->addNewCamera("defaultCamera",_camera);
	 //We have an A.I. thread, a networking thread, a PhysX thread, the main update/rendering thread
	 //so how many threads do we allocate for tasks? That's up to the programmer to decide for each app
	 //we add the A.I. thread in the same pool as it's a task. ReCast should also use this ...
	 _mainTaskPool = New boost::threadpool::pool(THREAD_LIMIT + 1 /*A.I.*/);
}

Kernel::~Kernel(){
	SAFE_DELETE(_cameraMgr);
	_inputInterface.terminate();
	_inputInterface.DestroyInstance();
    _mainTaskPool->wait();
	SAFE_DELETE(_mainTaskPool);
}

void Kernel::Idle(){
	GFX_DEVICE.idle();
	PHYSICS_DEVICE.idle();
	PostFX::getInstance().idle();
	SceneManager::getInstance().idle();
	LightManager::getInstance().idle();
	ShaderManager::getInstance().idle();
	FrameListenerManager::getInstance().idle();

	std::string pendingLanguage = ParamHandler::getInstance().getParam<std::string>("language");
	if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
		Locale::changeLanguage(pendingLanguage);
	}
}

void Kernel::MainLoopApp(){
	// Update time at every render loop
	Kernel::_currentTimeMS   = GETMSTIME();
	Kernel::_currentTime     = getMsToSec(Kernel::_currentTimeMS);
	FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
	Application& APP = Application::getInstance();

	if(!_keepAlive || APP.ShutdownRequested()) {
		//exiting the graphical rendering loop will return us to the current control point
		//if we return here, the next valid control point is in main() thus shutding down the application neatly
		GFX_DEVICE.exitRenderLoop(true);
		return;
	}

	Kernel::Idle();
	//Restore CPU to default state: clear buffers and set default render state
	GFX_DEVICE.beginFrame();
	//Get current frame-to-application speed factor
	Framerate::getInstance().SetSpeedFactor();
	//Launch the FRAME_STARTED event
	FrameEvent evt;
	frameMgr.createEvent(FRAME_EVENT_STARTED,evt);
	_keepAlive = frameMgr.frameStarted(evt);
	//Process physics
	PHYSICS_DEVICE.process();
	//Process the current frame
	_keepAlive = APP.getKernel()->MainLoopScene();
	//Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
	frameMgr.createEvent(FRAME_EVENT_PROCESS,evt);
	_keepAlive = frameMgr.frameRenderingQueued(evt);

	GFX_DEVICE.endFrame();
	//Launch the FRAME_ENDED event (buffers have been swapped)
	frameMgr.createEvent(FRAME_EVENT_ENDED,evt);
	_keepAlive = frameMgr.frameEnded(evt);
}

void Kernel::FirstLoop(){
	//Skip one frame so all resources can be built while the splash screen is displayed
	MainLoopApp();
	Kernel::_applicationReady = true;
	Framerate::getInstance().benchmark(true);
	//Hide splash screen
	ParamHandler& par = ParamHandler::getInstance();
	GFX_DEVICE.changeResolution(par.getParam<U16>("runtime.resolutionWidth"),
							    par.getParam<U16>("runtime.resolutionHeight"));
	GFX_DEVICE.setWindowPos(10,50);
	//Bind main render loop
	_mainLoopCallback = DELEGATE_REF(Kernel::MainLoopApp);
}

const I32 SKIP_TICKS = 1000 / TICKS_PER_SECOND;
bool Kernel::MainLoopScene(){
    if(_renderingPaused) {
        Idle();
        return true;
    }
	//Update camera position
	_camera->RenderLookAt();
	//Set the current scene's active camera
	_sceneMgr.updateCamera(_camera);
	_loops = 0;
	//while(_currentTimeMS > _nextGameTick && _loops < MAX_FRAMESKIP) {
		// Update scene based on input
		_sceneMgr.processInput();
		// process all scene events
		_sceneMgr.processTasks(_currentTimeMS);

		//Update the scene state based on current time
		_sceneMgr.updateSceneState(_currentTimeMS);
		//Update physics
		_PFX.update();

		_nextGameTick += SKIP_TICKS;
        _loops++;
	//}

	bool sceneRenderState = presentToScreen();
	// Get input events
	_inputInterface.tick();
	// Draw the GUI
	_GUI.draw(_currentTimeMS);
	return sceneRenderState;
}

bool Kernel::presentToScreen(){
    //Prepare scene for rendering
	_sceneMgr.preRender();
	//perform time-sensitive shader tasks
	_shaderMgr.tick(_currentTimeMS);

	// Inform listeners that we finished pre-rendering
	FrameEvent evt;
	_frameMgr.createEvent(FRAME_PRERENDER_END,evt);
	if(!_frameMgr.framePreRenderEnded(evt)) return false;

	// Render the scene adding any post-processing effects that we have active
	PostFX::getInstance().render(_camera);
	
	_sceneMgr.postRender();

	//render debug primitives and cleanup after us
	_GFX.flush();
    return true;
}

I8 Kernel::Initialize(const std::string& entryPoint) {
	I8 windowId = -1;
	I8 returnCode = 0;
	ParamHandler& par = ParamHandler::getInstance();
	Console::getInstance().bindConsoleOutput(DELEGATE_BIND(&GUIConsole::printText, GUI::getInstance().getConsole(),_1,_2));
	//Using OpenGL for rendering as default
	_GFX.setApi(OpenGL);
	//Target FPS is usually 60. So all movement is capped around that value
	Framerate::getInstance().Init(TARGET_FRAME_RATE);
	//Load info from XML files
	std::string startupScene = XML::loadScripts(entryPoint);
	//Create mem log file
	std::string mem = par.getParam<std::string>("memFile");
	if(mem.compare("none") == 0) mem = "mem.log";
	_APP.setMemoryLogFile(mem);
	PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
	vec2<U16> resolution(par.getParam<U16>("runtime.resolutionWidth"),
						 par.getParam<U16>("runtime.resolutionHeight"));
	windowId = _GFX.initHardware(resolution/2,_argc,_argv);
	//If we could not initialize the graphics device, exit
	if(windowId < 0) return windowId;

    _GFX.setRenderStage(FINAL_STAGE);
	//Load the splash screen
    _GFX.changeResolution(400,300);
	GUISplash splashScreen("divideLogo.jpg",vec2<U16>(400,300));
	_GFX.beginFrame();
	splashScreen.render();
	_GFX.endFrame();
	//We start of with a forward renderer. Change it to something else on scene load ... or ... whenever
	_GFX.setRenderer(New ForwardRenderer());
	_GFX.registerKernel(this);
	_lightPool.init();
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
	//Bind the kernel with the input interface
	_inputInterface.initialize(this,par.getParam<std::string>("appTitle"),(size_t)_GFX.getHWND());
	//Load default material
	PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
	XML::loadMaterialXML(par.getParam<std::string>("scriptLocation")+"/defaultMaterial");
    //Initialize GUI with our current resolution
	GUI::getInstance().init();
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
	//Initialize and start the AI
	_sceneMgr.initializeAI(true);
	PRINT_FN(Locale::get("CREATE_AI_ENTITIES_END"));
	_GUI.cacheResolution(resolution);
	_nextGameTick = GETMSTIME();
	return windowId;
}

void Kernel::beginLogicLoop(){
	PRINT_FN(Locale::get("START_RENDER_LOOP"));
	//lock the scene
	GET_ACTIVE_SCENE()->state()->toggleRunningState(true);
	//The first loop compiles all the textures, so, do not render the first frame
	_mainLoopCallback = DELEGATE_REF(Kernel::FirstLoop);
	//Target FPS is define in "config.h". So all movement is capped around that value
	//start rendering
	GFX_DEVICE.initDevice(TARGET_FRAME_RATE);
}

void Kernel::Shutdown(){
	_keepAlive = false;
	//release the scene
	GET_ACTIVE_SCENE()->state()->toggleRunningState(false);
	PRINT_FN(Locale::get("STOP_GUI"));
    Console::getInstance().bindConsoleOutput(boost::function2<void, std::string, bool>());
	_GUI.DestroyInstance(); ///Deactivate GUI
	PRINT_FN(Locale::get("STOP_PHYSICS_INTERFACE"));
	_PFX.exitPhysics();
	PRINT_FN(Locale::get("STOP_POST_FX"));
	PostFX::getInstance().DestroyInstance();
	PRINT_FN(Locale::get("STOP_SCENE_MANAGER"));
	_sceneMgr.deinitializeAI(true);
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
	ShaderManager::getInstance().refresh();
	Application& app = Application::getInstance();
    //minimized
    if(w == 0 || h == 0){
        _renderingPaused = true;
    }else{
        _renderingPaused = false;
    }
	app.setResolutionWidth(w);
	app.setResolutionHeight(h);
	//Update post-processing render targets and buffers
	PostFX::getInstance().reshapeFBO(w, h);
	vec2<U16> newResolution(w,h);
	//Update the graphical user interface
	GUI::getInstance().onResize(newResolution);
	// Cache resolution for faster access
	SceneManager::getInstance().cacheResolution(newResolution);
}

///--------------------------Input Management-------------------------------------///

bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
	if(GUI::getInstance().keyCheck(key,true)){
		GET_ACTIVE_SCENE()->onKeyDown(key);
	}
	return true;
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
	if(GUI::getInstance().keyCheck(key,false)){
		GET_ACTIVE_SCENE()->onKeyUp(key);
	}
	return true;
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
	if(GUI::getInstance().checkItem(arg)){
		GET_ACTIVE_SCENE()->onMouseMove(arg);
	}
	return true;
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUI::getInstance().clickCheck(button,true)){
		GET_ACTIVE_SCENE()->onMouseClickDown(arg,button);
	}
	return true;
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
	if(GUI::getInstance().clickCheck(button,false)){
		GET_ACTIVE_SCENE()->onMouseClickUp(arg,button);
	}
	return true;
}

bool Kernel::onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone) {
	GET_ACTIVE_SCENE()->onJoystickMoveAxis(arg,axis,deadZone);
	return true;
}

bool Kernel::onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
	GET_ACTIVE_SCENE()->onJoystickMovePOV(arg,pov);
	return true;
}

bool Kernel::onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
	GET_ACTIVE_SCENE()->onJoystickButtonDown(arg,button);
	return true;
}

bool Kernel::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
	GET_ACTIVE_SCENE()->onJoystickButtonUp(arg,button);
	return true;
}

bool Kernel::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
	GET_ACTIVE_SCENE()->sliderMoved(arg,index);
	return true;
}

bool Kernel::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
	GET_ACTIVE_SCENE()->vector3Moved(arg,index);
	return true;
}