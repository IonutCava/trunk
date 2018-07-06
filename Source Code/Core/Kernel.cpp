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
#include "Hardware/Video/Headers/RenderStateBlock.h"

#include "Rendering/Headers/ForwardRenderer.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"
#include "Rendering/Headers/DeferredLightingRenderer.h"

U64 Kernel::_previousTime = 0ULL;
U64 Kernel::_currentTime = 0ULL;
U64 Kernel::_currentTimeDelta = 0ULL;
D32 Kernel::_nextGameTick = 0.0;

bool Kernel::_keepAlive = true;
bool Kernel::_applicationReady = false;
bool Kernel::_renderingPaused = false;

DELEGATE_CBK Kernel::_mainLoopCallback;
ProfileTimer* s_appLoopTimer = NULL;

#if USE_FIXED_TIMESTEP
static const U64 SKIP_TICKS = (1000 * 1000) / Config::TICKS_PER_SECOND;
#endif

Kernel::Kernel(I32 argc, char **argv, Application& parentApp) :
                    _argc(argc),
                    _argv(argv),
                    _APP(parentApp),
                    _GFX(GFXDevice::getOrCreateInstance()), //Video
                    _SFX(SFXDevice::getOrCreateInstance()), //Audio
                    _PFX(PXDevice::getOrCreateInstance()),  //Physics
                    _GUI(GUI::getOrCreateInstance()),       //Graphical User Interface
                    _cameraMgr(New CameraManager()),               //Camera manager
                    _sceneMgr(SceneManager::getOrCreateInstance()) //Scene Manager
                    
{
    ResourceCache::createInstance();
    FrameListenerManager::createInstance();
    InputInterface::createInstance();

    //General light management and rendering (individual lights are handled by each scene)
    //Unloading the lights is a scene level responsibility
    LightManager::createInstance();

    assert(_cameraMgr != NULL);
    //If camera has been changed, set a callback to inform the current scene
    _cameraMgr->addCameraChangeListener(DELEGATE_BIND(&SceneManager::updateCameras, //update camera
                                                      DELEGATE_REF(_sceneMgr)));
    // force all lights to update on camera change (to keep them still actually)
    _cameraMgr->addCameraUpdateListener(DELEGATE_BIND(&LightManager::update, 
                                                       DELEGATE_REF(LightManager::getInstance()),
                                                       true));
    //We have an A.I. thread, a networking thread, a PhysX thread, the main update/rendering thread
    //so how many threads do we allocate for tasks? That's up to the programmer to decide for each app
    //we add the A.I. thread in the same pool as it's a task. ReCast should also use this ...
    _mainTaskPool = New boost::threadpool::pool(Config::THREAD_LIMIT + 1 /*A.I.*/);
    s_appLoopTimer = ADD_TIMER("MainLoopTimer");
}

Kernel::~Kernel(){
    SAFE_DELETE(_cameraMgr);
    _mainTaskPool->wait();
    SAFE_DELETE(_mainTaskPool);
    REMOVE_TIMER(s_appLoopTimer);
}

void Kernel::idle(){
    GFX_DEVICE.idle();
    PHYSICS_DEVICE.idle();
    SceneManager::getInstance().idle();
    LightManager::getInstance().idle();
    FrameListenerManager::getInstance().idle();

    std::string pendingLanguage = ParamHandler::getInstance().getParam<std::string>("language");
    if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
        Locale::changeLanguage(pendingLanguage);
    }
}

void Kernel::mainLoopApp(){
    // Update internal timer
    ApplicationTimer::getInstance().update();

    START_TIMER(s_appLoopTimer);

    // Update time at every render loop
    Kernel::_previousTime    = Kernel::_currentTime;
    Kernel::_currentTime      = GETUSTIME();
    Kernel::_currentTimeDelta = Kernel::_currentTime - Kernel::_previousTime;

    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
    Application& APP = Application::getInstance();

    if(!_keepAlive || APP.ShutdownRequested()) {
        //exiting the graphical rendering loop will return us to the current control point
        //if we return here, the next valid control point is in main() thus shutding down the application neatly
        GFX_DEVICE.exitRenderLoop(true);
        return;
    }

    Kernel::idle();
    //Restore GPU to default state: clear buffers and set default render state
    GFX_DEVICE.beginFrame();
    //Launch the FRAME_STARTED event
    FrameEvent evt;
    frameMgr.createEvent(_currentTime, FRAME_EVENT_STARTED, evt);
    _keepAlive = frameMgr.frameStarted(evt);

    //Process the current frame
    _keepAlive = APP.getKernel()->mainLoopScene(evt);
    //Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
    frameMgr.createEvent(_currentTime, FRAME_EVENT_PROCESS,evt);
    _keepAlive = frameMgr.frameRenderingQueued(evt);

    GFX_DEVICE.endFrame();
    //Launch the FRAME_ENDED event (buffers have been swapped)
    frameMgr.createEvent(_currentTime, FRAME_EVENT_ENDED,evt);
    _keepAlive = frameMgr.frameEnded(evt);

    STOP_TIMER(s_appLoopTimer);

#if defined(_DEBUG) || defined(_PROFILE)  
    static I32 count = 0;
    count++;
    if(count > 600){
        PRINT_FN("GPU: [ %5.5f ]", getUsToSec(GFX_DEVICE.getFrameDurationGPU()));
        count = 0;
    }
#endif
}

bool Kernel::mainLoopScene(FrameEvent& evt){
    if(_renderingPaused) {
        idle();
        return true;
    }
    
    //Process physics
    _PFX.process(_currentTimeDelta);

#if USE_FIXED_TIMESTEP
    _loops = 0;
    U64 deltaTime = SKIP_TICKS;
    while(_currentTime > _nextGameTick && _loops < Config::MAX_FRAMESKIP) {
#else
    U64 deltaTime = _currentTimeDelta;
#endif
        _cameraMgr->update(deltaTime);

        // Update scene based on input
        _sceneMgr.processInput(deltaTime);
 
        // process all scene events
        _sceneMgr.processTasks(deltaTime);

        // Get input events
        if(Application::getInstance().hasFocus()) {
            InputInterface::getInstance().update(deltaTime);
        }

#if USE_FIXED_TIMESTEP
        _nextGameTick += deltaTime;
        _loops++;

        if(_loops == Config::MAX_FRAMESKIP && _currentTime > _nextGameTick)
            _nextGameTick = _currentTime;
    }
#endif
         
    // Call this to avoid interpolating 60 bone matrices per entity every render call
    // Update the scene state based on current time (e.g. animation matrices)
    _sceneMgr.updateSceneState(_currentTimeDelta);

    //Update physics - uses own timestep implementation
    _PFX.update(_currentTimeDelta);
#if USE_FIXED_TIMESTEP
    return presentToScreen(evt, std::min(static_cast<D32>((_currentTime + deltaTime - _nextGameTick )) / static_cast<D32>(deltaTime), 1.0));
#else
    return presentToScreen(evt, 1.0);
#endif
}

void Kernel::displayScene(){
     RenderStage stage = (_GFX.getRenderer()->getType() != RENDERER_FORWARD) ? DEFERRED_STAGE : FINAL_STAGE;
     bool postProcessing = (stage != DEFERRED_STAGE && _GFX.postProcessingEnabled());

    _cameraMgr->getActiveCamera()->renderLookAt();

    /*
    FrameBufferObject::FrameBufferObjectTarget depthPassPolicy, colorPassPolicy;
    depthPassPolicy._depthOnly = true;
    colorPassPolicy._colorOnly = true;
    */
    assert(_GFX.getScreenBuffer(0) != NULL);

    _GFX.isDepthPrePass(true);

    // Z-prePass
    _GFX.getDepthBuffer()->Begin(FrameBufferObject::defaultPolicy());
        SceneManager::getInstance().render(Z_PRE_PASS, *this);
   
    _GFX.isDepthPrePass(false);

    if(!postProcessing){
        _GFX.getDepthBuffer()->End();
    }else{
        _GFX.getScreenBuffer(0)->Begin(FrameBufferObject::defaultPolicy());        
    }
        SceneManager::getInstance().render(stage, *this);

    if(postProcessing){
        _GFX.getScreenBuffer(0)->End();
        PostFX::getInstance().displaySceneWithoutAnaglyph();
    }
    
}

void Kernel::displaySceneAnaglyph(){
    RenderStage stage = (_GFX.getRenderer()->getType() != RENDERER_FORWARD) ? DEFERRED_STAGE : FINAL_STAGE;
    /*
    FrameBufferObject::FrameBufferObjectTarget depthPassPolicy, colorPassPolicy;
    depthPassPolicy._depthOnly = true;
    colorPassPolicy._colorOnly = true;
    */
    Camera* currentCamera = _cameraMgr->getActiveCamera();
    currentCamera->saveCamera();

    // Render to right eye
    currentCamera->setAnaglyph(true);
    currentCamera->renderLookAt();
        // Z-prePass
        _GFX.getDepthBuffer()->Begin(FrameBufferObject::defaultPolicy());
            SceneManager::getInstance().render(Z_PRE_PASS, *this); 

        // first screen buffer
        _GFX.getScreenBuffer(0)->Begin(FrameBufferObject::defaultPolicy());
            SceneManager::getInstance().render(stage, *this);


    // Render to left eye
    currentCamera->setAnaglyph(false);
    currentCamera->renderLookAt();
        // Z-prePass
        _GFX.getDepthBuffer()->Begin(FrameBufferObject::defaultPolicy());
            SceneManager::getInstance().render(Z_PRE_PASS, *this);
        // second screen buffer
        _GFX.getScreenBuffer(1)->Begin(FrameBufferObject::defaultPolicy());
            SceneManager::getInstance().render(stage, *this);
        _GFX.getScreenBuffer(1)->End();

    currentCamera->restoreCamera();
    PostFX::getInstance().displaySceneWithAnaglyph();
}


bool Kernel::presentToScreen(FrameEvent& evt, const D32 interpolationFactor){
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();

    frameMgr.createEvent(_currentTime, FRAME_PRERENDER_START,evt);
    if(!frameMgr.framePreRenderStarted(evt)) return false;

    _GFX.setInterpolation(interpolationFactor);

    //Prepare scene for rendering
    _sceneMgr.preRender();

    //perform time-sensitive shader tasks
    ShaderManager::getInstance().update(_currentTimeDelta);

    frameMgr.createEvent(_currentTime, FRAME_PRERENDER_END,evt);
    if(!frameMgr.framePreRenderEnded(evt)) return false;

    _GFX.anaglyphEnabled() ? displaySceneAnaglyph() : displayScene();

    _sceneMgr.postRender();

    //render debug primitives and cleanup after us
    _GFX.flush();

    // Draw the GUI
    _GUI.draw(_currentTimeDelta, interpolationFactor);

    return true;
}

void Kernel::firstLoop(){
    static bool first = true;

    ParamHandler& par = ParamHandler::getInstance();
    static bool shadowMappingEnabled = par.getParam<bool>("rendering.enableShadows");
    //Skip one frame so all resources can be built while the splash screen is displayed
    if(first){
        par.setParam("rendering.enableShadows",false);
        mainLoopApp();
        Kernel::_applicationReady = true;
        //Hide splash screen
        GFX_DEVICE.changeResolution(par.getParam<U16>("runtime.resolutionWidth"),
                                    par.getParam<U16>("runtime.resolutionHeight"));
        GFX_DEVICE.setWindowPos(10,50);
        first = false;
    }else{// skip another frame so all buffers and shaders are refreshed
        par.setParam("rendering.enableShadows", shadowMappingEnabled);
        mainLoopApp();
        _mainLoopCallback = DELEGATE_REF(Kernel::mainLoopApp);
#if defined(_DEBUG) || defined(_PROFILE)
        ApplicationTimer::getInstance().benchmark(true);
#endif
    }

    _currentTime = _nextGameTick = GETUSTIME();
}

void Kernel::submitRenderCall(const RenderStage& stage, const SceneRenderState& sceneRenderState, const DELEGATE_CBK& sceneRenderCallback) const {
    _GFX.setRenderStage(stage);
    _GFX.render(sceneRenderCallback, sceneRenderState);

    if(bitCompare(stage,FINAL_STAGE) || bitCompare(stage,DEFERRED_STAGE)){
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        _GFX.debugDraw();
        // Show navmeshes
        AIManager::getInstance().debugDraw(false);
    }
}

I8 Kernel::initialize(const std::string& entryPoint) {
    I8 windowId = -1;
    I8 returnCode = 0;
    ParamHandler& par = ParamHandler::getInstance();

    Console::getInstance().bindConsoleOutput(DELEGATE_BIND(&GUIConsole::printText, GUI::getInstance().getConsole(), _1, _2));
    //Using OpenGL for rendering as default
    _GFX.setApi(OpenGL);

    //Target FPS is usually 60. So all movement is capped around that value
    ApplicationTimer::getInstance().init(Config::TARGET_FRAME_RATE);

    //Load info from XML files
    std::string startupScene = XML::loadScripts(entryPoint);

    //Create mem log file
    std::string mem = par.getParam<std::string>("memFile");
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);

    PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
    vec2<U16> resolution(par.getParam<U16>("runtime.resolutionWidth"),
                         par.getParam<U16>("runtime.resolutionHeight"));

    _GFX.registerKernel(this);
    windowId = _GFX.initHardware(resolution/2,_argc,_argv);

    //If we could not initialize the graphics device, exit
    if(windowId < 0) return windowId;

    //We start of with a forward renderer. Change it to something else on scene load ... or ... whenever
    _GFX.setRenderer(New ForwardRenderer());

    //Load and render the splash screen
    _GFX.setRenderStage(FINAL_STAGE);
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg",vec2<U16>(400,300)).render();
    _GFX.endFrame();

    LightManager::getInstance().init();

    PRINT_FN(Locale::get("START_SOUND_INTERFACE"));
    if((returnCode = _SFX.initHardware()) != NO_ERR)
        return returnCode;

    PRINT_FN(Locale::get("START_PHYSICS_INTERFACE"));
    if((returnCode =_PFX.initPhysics(Config::TARGET_FRAME_RATE)) != NO_ERR)
        return returnCode;

    PostFX::getInstance().init(resolution);

    //Bind the kernel with the input interface
    InputInterface::getInstance().initialize(this, par.getParam<std::string>("appTitle"), (size_t)_GFX.getHWND());

    //Initialize GUI with our current resolution
    _GUI.init();
    _GUI.cacheResolution(resolution);

    _sceneMgr.init(&_GUI);

    if(!_sceneMgr.load(startupScene, resolution, _cameraMgr)){       //< Load the scene
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

    return windowId;
}

void Kernel::beginLogicLoop(){
    PRINT_FN(Locale::get("START_RENDER_LOOP"));
    Kernel::_nextGameTick = GETUSTIME();
    //lock the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(true);
    //The first loop compiles all the textures, so, do not render the first frame
    _mainLoopCallback = DELEGATE_REF(Kernel::firstLoop);
    //Target FPS is define in "config.h". So all movement is capped around that value
    //start rendering
    GFX_DEVICE.initDevice(Config::TARGET_FRAME_RATE);
}

void Kernel::shutdown(){
    _keepAlive = false;
    //release the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(false);
    PRINT_FN(Locale::get("STOP_GUI"));
    Console::getInstance().bindConsoleOutput(boost::function2<void, const char*, bool>());
    _GUI.destroyInstance(); ///Deactivate GUI
    PRINT_FN(Locale::get("STOP_SCENE_MANAGER"));
    _sceneMgr.unloadCurrentScene();
    _sceneMgr.deinitializeAI(true);
    _sceneMgr.destroyInstance();
    _GFX.closeRenderer();
    PRINT_FN(Locale::get("STOP_RESOURCE_CACHE"));
    ResourceCache::getInstance().destroyInstance();
    LightManager::getInstance().destroyInstance();
    PRINT_FN(Locale::get("STOP_ENGINE_OK"));
    FrameListenerManager::getInstance().destroyInstance();
    PRINT_FN(Locale::get("STOP_PHYSICS_INTERFACE"));
    _PFX.exitPhysics();
    PRINT_FN(Locale::get("STOP_HARDWARE"));
    InputInterface::getInstance().destroyInstance();
    _SFX.closeAudioApi();
    _SFX.destroyInstance();
    _GFX.closeRenderingApi();
    _GFX.destroyInstance();
    Locale::clear();
}

void Kernel::updateResolutionCallback(I32 w, I32 h){
    if(!_applicationReady) return;
    Application& app = Application::getInstance();
    //minimized
    _renderingPaused = (w == 0 || h == 0);
    app.setResolutionWidth(w);
    app.setResolutionHeight(h);
    app.isFullScreen(!ParamHandler::getInstance().getParam<bool>("runtime.windowedMode"));
    vec2<U16> newResolution(w,h);
    //Update the graphical user interface
    GUI::getInstance().onResize(newResolution);
    // Cache resolution for faster access
    SceneManager::getInstance().cacheResolution(newResolution);
    // Update internal resolution tracking (used for joysticks and mouse)
    InputInterface::getInstance().updateResolution(w,h);
    //Update post-processing render targets and buffers
    PostFX::getInstance().reshapeFBO(w, h);
    ShaderManager::getInstance().refresh();
}

///--------------------------Input Management-------------------------------------///
bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
    return _sceneMgr.onKeyDown(key);
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
    return _sceneMgr.onKeyUp(key);
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
    _cameraMgr->onMouseMove(arg);
    return _sceneMgr.onMouseMove(arg);
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    return _sceneMgr.onMouseClickDown(arg,button);
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    return _sceneMgr.onMouseClickUp(arg,button);
}

bool Kernel::onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone) {
    return _sceneMgr.onJoystickMoveAxis(arg,axis,deadZone);
}

bool Kernel::onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
    return _sceneMgr.onJoystickMovePOV(arg,pov);
}

bool Kernel::onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
    return _sceneMgr.onJoystickButtonDown(arg,button);
}

bool Kernel::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
    return _sceneMgr.onJoystickButtonUp(arg,button);
}

bool Kernel::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    return _sceneMgr.sliderMoved(arg,index);
}

bool Kernel::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
    return _sceneMgr.vector3Moved(arg,index);
}