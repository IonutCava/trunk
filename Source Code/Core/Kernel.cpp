#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Shaders/Headers/ShaderManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

U64 Kernel::_previousTime = 0ULL;
U64 Kernel::_currentTime = 0ULL;
U64 Kernel::_currentTimeFrozen = 0ULL;
U64 Kernel::_currentTimeDelta = 0ULL;
D32 Kernel::_nextGameTick = 0.0;

bool Kernel::_keepAlive = true;
bool Kernel::_renderingPaused = false;
bool Kernel::_freezeLoopTime = false;
bool Kernel::_freezeGUITime = false;
ProfileTimer* s_appLoopTimer = nullptr;

boost::lockfree::queue<U64, boost::lockfree::capacity<Config::THREAD_LIMIT + 1> >  Kernel::_callbackBuffer;
Unordered_map<U64, DELEGATE_CBK > Kernel::_threadedCallbackFunctions;

#if defined(USE_FIXED_TIMESTEP)
static const U64 SKIP_TICKS = (1000 * 1000) / Config::TICKS_PER_SECOND;
#endif

Kernel::Kernel(I32 argc, char **argv, Application& parentApp) :
                    _argc(argc),
                    _argv(argv),
                    _APP(parentApp),
                    _GFX(GFXDevice::getOrCreateInstance()), //Video
                    _SFX(SFXDevice::getOrCreateInstance()), //Audio
                    _PFX(PXDevice::getOrCreateInstance()),  //Physics
                    _input(InputInterface::getOrCreateInstance()), //Input
                    _GUI(GUI::getOrCreateInstance()),       //Graphical User Interface
                    _sceneMgr(SceneManager::getOrCreateInstance()) //Scene Manager
                    
{
    ResourceCache::createInstance();
    FrameListenerManager::createInstance();
    
    //General light management and rendering (individual lights are handled by each scene)
    //Unloading the lights is a scene level responsibility
    LightManager::createInstance();
    _cameraMgr = New CameraManager(this);               //Camera manager
    assert(_cameraMgr != nullptr);
    // force all lights to update on camera change (to keep them still actually)
    _cameraMgr->addCameraUpdateListener(DELEGATE_BIND(&LightManager::onCameraChange, DELEGATE_REF(LightManager::getInstance())));
    _cameraMgr->addCameraUpdateListener(DELEGATE_BIND(&SceneManager::onCameraChange, DELEGATE_REF(SceneManager::getInstance())));
    //We have an A.I. thread, a networking thread, a PhysX thread, the main update/rendering thread
    //so how many threads do we allocate for tasks? That's up to the programmer to decide for each app
    //we add the A.I. thread in the same pool as it's a task. ReCast should also use this ...
    _mainTaskPool = New boost::threadpool::pool(Config::THREAD_LIMIT + 1 /*A.I.*/);

    s_appLoopTimer = ADD_TIMER("MainLoopTimer");
}

Kernel::~Kernel()
{
    _mainTaskPool->wait();
    SAFE_DELETE(_mainTaskPool);
    REMOVE_TIMER(s_appLoopTimer);
}

void Kernel::threadPoolCompleted(U64 onExitTaskID) {
    while (!_callbackBuffer.push(onExitTaskID)) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }
}

void Kernel::idle(){
    GFX_DEVICE.idle();
    PHYSICS_DEVICE.idle();
    SceneManager::getInstance().idle();
    LightManager::getInstance().idle();
    FrameListenerManager::getInstance().idle();

    ParamHandler& par = ParamHandler::getInstance();

    _freezeGUITime  = par.getParam("freezeGUITime", false);
    bool freezeLoopTime = par.getParam("freezeLoopTime", false);
    if (freezeLoopTime != _freezeLoopTime) {
        _freezeLoopTime = freezeLoopTime;
        _currentTimeFrozen = _currentTime;
        AIManager::getInstance().pauseUpdate(freezeLoopTime);
    }

    std::string pendingLanguage = par.getParam<std::string>("language");
    if(pendingLanguage.compare(Locale::currentLanguage()) != 0){
        Locale::changeLanguage(pendingLanguage);
    }

    if (!_callbackBuffer.empty()) {
        U64 onExitTaskID;
        if (_callbackBuffer.pop(onExitTaskID)) {
            DELEGATE_CBK& cbk = _threadedCallbackFunctions[onExitTaskID];
            if (!cbk.empty()) {
                cbk();
            }
        }
    }
}

void Kernel::mainLoopApp() {
    if(!_keepAlive) {
        // exiting the rendering loop will return us to the last control point (i.e. Kernel::runLogicLoop)
        Application::getInstance().mainLoopActive(false);
        return;
    }

    // Update internal timer
    ApplicationTimer::getInstance().update(GFX_DEVICE.getFrameCount());

    START_TIMER(s_appLoopTimer);

    // Update time at every render loop
    _previousTime     = _currentTime;
    _currentTime      = GETUSTIME();
    _currentTimeDelta = _currentTime - _previousTime;

    FrameEvent evt;
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
    Application& APP = Application::getInstance();

    Kernel::idle();

    //Restore GPU to default state: clear buffers and set default render state
    GFX_DEVICE.beginFrame();
    {
        //Launch the FRAME_STARTED event
        frameMgr.createEvent(_currentTime, FRAME_EVENT_STARTED, evt);
        _keepAlive = frameMgr.frameEvent(evt);

        //Process the current frame
        _keepAlive = APP.getKernel()->mainLoopScene(evt) && _keepAlive;

        //Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
        frameMgr.createEvent(_currentTime, FRAME_EVENT_PROCESS, evt);
        _keepAlive = frameMgr.frameEvent(evt) && _keepAlive;
    }
    GFX_DEVICE.endFrame();

    //Launch the FRAME_ENDED event (buffers have been swapped)
    frameMgr.createEvent(_currentTime, FRAME_EVENT_ENDED,evt);
    _keepAlive = frameMgr.frameEvent(evt) && _keepAlive;

    _keepAlive = !APP.ShutdownRequested() && _keepAlive;

    STOP_TIMER(s_appLoopTimer);

#if defined(_DEBUG) || defined(_PROFILE)  
    if (GFX_DEVICE.getFrameCount() % (Config::TARGET_FRAME_RATE * 10) == 0){
        PRINT_FN("GPU: [ %5.5f ] [DrawCalls: %d]", getUsToSec(GFX_DEVICE.getFrameDurationGPU()), GFX_DEVICE.getDrawCallCount());
    }
#endif
}

bool Kernel::mainLoopScene(FrameEvent& evt){
    if(_renderingPaused) {
        idle();
        return true;
    }
    
    //Process physics
    _PFX.process(_freezeLoopTime ? 0ULL : _currentTimeDelta);

    _loops = 0;

#if !defined(USE_FIXED_TIMESTEP)
    U64 deltaTime = _currentTimeDelta;
#else
    U64 deltaTime = SKIP_TICKS;
    while (_currentTime > _nextGameTick && _loops < Config::MAX_FRAMESKIP) {    
#endif

        if (!_freezeGUITime) {
            _sceneMgr.processGUI(deltaTime);
        }

        if (!_freezeLoopTime) { 
            // Update scene based on input
            _sceneMgr.processInput(_currentTimeDelta);
            // process all scene events
            _sceneMgr.processTasks(deltaTime);
        }

        _nextGameTick += deltaTime;
        _loops++;

#if defined(USE_FIXED_TIMESTEP)
        if (_loops == Config::MAX_FRAMESKIP && _currentTime > _nextGameTick) {
            _nextGameTick = _currentTime;
        }
    } // while

    _GFX.setInterpolation(std::min(static_cast<D32>((_currentTime + deltaTime - _nextGameTick)) / static_cast<D32>(deltaTime), 1.0));
#endif
    
    // Get input events
    _APP.hasFocus() ? _input.update(deltaTime) : _sceneMgr.onLostFocus();

    // Call this to avoid interpolating 60 bone matrices per entity every render call
    // Update the scene state based on current time (e.g. animation matrices)
    _sceneMgr.updateSceneState(_freezeLoopTime ? 0ULL : _currentTimeDelta);

    // Update physics - uses own timestep implementation
    _PFX.update(_freezeLoopTime ? 0ULL : _currentTimeDelta);

    // Update the graphical user interface
    GUI::getInstance().update(_freezeGUITime ? 0ULL : _currentTimeDelta);

    return presentToScreen(evt);
}

void Kernel::renderScene() {
    RenderStage stage = (_GFX.getRenderer()->getType() != RENDERER_FORWARD_PLUS) ? DEFERRED_STAGE : FINAL_STAGE;
    bool postProcessing = (stage != DEFERRED_STAGE && _GFX.postProcessingEnabled());

    if (_GFX.anaglyphEnabled() && postProcessing) {
        renderSceneAnaglyph();
        return;
    }

    Framebuffer::FramebufferTarget depthPassPolicy, colorPassPolicy;
    depthPassPolicy._depthOnly = true;
    colorPassPolicy._colorOnly = true;
    
    /// Lock the render pass manager because the Z_PRE_PASS and the FINAL_STAGE pass must render the exact same geometry
    RenderPassManager::getInstance().lock();
    // Z-prePass
    _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Begin(Framebuffer::defaultPolicy());
        _sceneMgr.render(Z_PRE_PASS_STAGE, *this);
    _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->End();

    _GFX.ConstructHIZ();

    if (postProcessing) {
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Begin(Framebuffer::defaultPolicy());
    }

    _sceneMgr.render(stage, *this);

    if (postProcessing) {
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->End();
        PostFX::getInstance().displayScene();
    }

    RenderPassManager::getInstance().unlock();
}

void Kernel::renderSceneAnaglyph(){
    RenderStage stage = (_GFX.getRenderer()->getType() != RENDERER_FORWARD_PLUS) ? DEFERRED_STAGE : FINAL_STAGE;

    Framebuffer::FramebufferTarget depthPassPolicy, colorPassPolicy;
    depthPassPolicy._depthOnly = true;
    colorPassPolicy._colorOnly = true;
    
    Camera* currentCamera = _cameraMgr->getActiveCamera();

    // Render to right eye
    currentCamera->setAnaglyph(true);
    currentCamera->renderLookAt();
        RenderPassManager::getInstance().lock();
        // Z-prePass
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Begin(Framebuffer::defaultPolicy());
            SceneManager::getInstance().render(Z_PRE_PASS_STAGE, *this); 
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->End();
        // first screen buffer
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->Begin(Framebuffer::defaultPolicy());
            SceneManager::getInstance().render(stage, *this);
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->End();

        RenderPassManager::getInstance().unlock();

    // Render to left eye
    currentCamera->setAnaglyph(false);
    currentCamera->renderLookAt();
        RenderPassManager::getInstance().lock();
        // Z-prePass
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->Begin(Framebuffer::defaultPolicy());
            SceneManager::getInstance().render(Z_PRE_PASS_STAGE, *this);
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_DEPTH)->End();
        // second screen buffer
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_ANAGLYPH)->Begin(Framebuffer::defaultPolicy());
            SceneManager::getInstance().render(stage, *this);
        _GFX.getRenderTarget(GFXDevice::RENDER_TARGET_ANAGLYPH)->End();

        RenderPassManager::getInstance().unlock();

    PostFX::getInstance().displayScene();
}


bool Kernel::presentToScreen(FrameEvent& evt) {
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();

    frameMgr.createEvent(_currentTime, FRAME_PRERENDER_START, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    //Prepare scene for rendering
    _sceneMgr.preRender();

    //perform time-sensitive shader tasks
    ShaderManager::getInstance().update(_currentTimeDelta);

    frameMgr.createEvent(_currentTime, FRAME_PRERENDER_END, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    renderScene();

    frameMgr.createEvent(_currentTime, FRAME_POSTRENDER_START, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    _sceneMgr.postRender();

    frameMgr.createEvent(_currentTime, FRAME_POSTRENDER_END, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    return true;
}

void Kernel::firstLoop() {
    ParamHandler& par = ParamHandler::getInstance();
    bool shadowMappingEnabled = par.getParam<bool>("rendering.enableShadows");
    //Skip two frames, one without and one with shadows, so all resources can be built while the splash screen is displayed
    par.setParam("freezeGUITime", true);
    par.setParam("freezeLoopTime", true);
    par.setParam("rendering.enableShadows",false);
    mainLoopApp();
    if (shadowMappingEnabled) {
        par.setParam("rendering.enableShadows", true);
        mainLoopApp();
    }
    GFX_DEVICE.setWindowPos(10, 60);
    par.setParam("freezeGUITime", false);
    par.setParam("freezeLoopTime", false);

    GFX_DEVICE.changeResolution(Application::getInstance().getPreviousResolution());
#if defined(_DEBUG) || defined(_PROFILE)
    ApplicationTimer::getInstance().benchmark(true);
#endif
    SceneManager::getInstance().initPostLoadState();

    _currentTime = _nextGameTick = GETUSTIME();
}

void Kernel::submitRenderCall(const RenderStage& stage, const SceneRenderState& sceneRenderState, const DELEGATE_CBK& sceneRenderCallback) const {
    _GFX.setRenderStage(stage);
    _GFX.getRenderer()->render(sceneRenderCallback, sceneRenderState);
}

I8 Kernel::initialize(const std::string& entryPoint) {
    ParamHandler& par = ParamHandler::getInstance();

    Console::getInstance().bindConsoleOutput(DELEGATE_BIND(&GUIConsole::printText, GUI::getInstance().getConsole(), _1, _2));
    //Using OpenGL for rendering as default
    _GFX.setApi(OpenGL);
    _GFX.setStateChangeExclusionMask(TYPE_LIGHT | TYPE_TRIGGER | TYPE_PARTICLE_EMITTER | TYPE_SKY | TYPE_VEGETATION_GRASS | TYPE_VEGETATION_TREES);
    //Target FPS is usually 60. So all movement is capped around that value
    ApplicationTimer::getInstance().init(Config::TARGET_FRAME_RATE);

    //Load info from XML files
    std::string startupScene = XML::loadScripts(entryPoint);

    //Create mem log file
    std::string mem = par.getParam<std::string>("memFile");
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);

    PRINT_FN(Locale::get("START_RENDER_INTERFACE"));
    vec2<U16> resolution = _APP.getResolution();
    F32 aspectRatio = (F32)resolution.width / (F32)resolution.height;
    _GFX.registerKernel(this);
    ErrorCodes initError = _GFX.initRenderingApi(vec2<U16>(400, 300), _argc, _argv);

    //If we could not initialize the graphics device, exit
    if(initError != NO_ERR) {
        return initError;
    }

    PRINT_FN(Locale::get("SCENE_ADD_DEFAULT_CAMERA"));
    Camera* camera = New FreeFlyCamera();
    camera->setProjection(aspectRatio, par.getParam<F32>("rendering.verticalFOV"), vec2<F32>(par.getParam<F32>("rendering.zNear"), par.getParam<F32>("rendering.zFar")));
    camera->setFixedYawAxis(true);
    //As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
    _cameraMgr->addNewCamera("defaultCamera", camera);
    _cameraMgr->pushActiveCamera(camera);

    //Load and render the splash screen
    _GFX.setRenderStage(FINAL_STAGE);
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg", vec2<U16>(400, 300)).render();
    _GFX.endFrame();

    LightManager::getInstance().init();

    PRINT_FN(Locale::get("START_SOUND_INTERFACE"));
    if((initError = _SFX.initAudioApi()) != NO_ERR) {
        return initError;
    }

    PRINT_FN(Locale::get("START_PHYSICS_INTERFACE"));
    if((initError =_PFX.initPhysicsApi(Config::TARGET_FRAME_RATE)) != NO_ERR) {
        return initError;
    }

    //Bind the kernel with the input interface
    InputInterface::getInstance().init(this, par.getParam<std::string>("appTitle"));

    //Initialize GUI with our current resolution
    _GUI.init(resolution);
    _sceneMgr.init(&_GUI);

    if(!_sceneMgr.load(startupScene, resolution, _cameraMgr)){       //< Load the scene
        ERROR_FN(Locale::get("ERROR_SCENE_LOAD"),startupScene.c_str());
        return MISSING_SCENE_DATA;
    }

    if(!_sceneMgr.checkLoadFlag()){
        ERROR_FN(Locale::get("ERROR_SCENE_LOAD_NOT_CALLED"),startupScene.c_str());
        return MISSING_SCENE_LOAD_CALL;
    }

    camera->setMoveSpeedFactor(par.getParam<F32>("options.cameraSpeed.move"));
    camera->setTurnSpeedFactor(par.getParam<F32>("options.cameraSpeed.turn"));

    PRINT_FN(Locale::get("INITIAL_DATA_LOADED"));
    PRINT_FN(Locale::get("CREATE_AI_ENTITIES_START"));
    //Initialize and start the AI
    _sceneMgr.initializeAI(true);
    PRINT_FN(Locale::get("CREATE_AI_ENTITIES_END"));

    return initError;
}

void Kernel::runLogicLoop(){
    PRINT_FN(Locale::get("START_RENDER_LOOP"));
    Kernel::_nextGameTick = GETUSTIME();
    //lock the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(true);
    //The first loops compiles all the visible data, so do not render the first couple of frames
    firstLoop();

    _keepAlive = true;
    _APP.mainLoopActive(true);

    while (_APP.mainLoopActive()) {
        Kernel::mainLoopApp();
    }

    shutdown();
}

void Kernel::shutdown(){
    //release the scene
    GET_ACTIVE_SCENE()->state().toggleRunningState(false);
    Console::getInstance().bindConsoleOutput(boost::function2<void, const char*, bool>());
    GUI::destroyInstance(); ///Deactivate GUI
    _sceneMgr.unloadCurrentScene();
    _sceneMgr.deinitializeAI(true);
    SceneManager::destroyInstance();
    // Close CEGUI
    try { 
        CEGUI::System::destroy();
    } catch( ... ) { 
        D_ERROR_FN(Locale::get("ERROR_CEGUI_DESTROY")); 
    }
    SAFE_DELETE(_cameraMgr);
    LightManager::destroyInstance();
    PRINT_FN(Locale::get("STOP_ENGINE_OK"));
    PRINT_FN(Locale::get("STOP_PHYSICS_INTERFACE"));
    _PFX.closePhysicsApi();
    PRINT_FN(Locale::get("STOP_HARDWARE"));
    _SFX.closeAudioApi();
    _GFX.closeRenderingApi();
    InputInterface::destroyInstance();
    SFXDevice::destroyInstance();
    GFXDevice::destroyInstance();
    Locale::clear();
    ResourceCache::destroyInstance();
    // Destroy the shader manager
    ShaderManager::destroyInstance();
    FrameListenerManager::destroyInstance();
}

void Kernel::updateResolutionCallback(I32 w, I32 h){
    Application& APP = Application::getInstance();
    APP.setResolution(w, h);
    // Update internal resolution tracking (used for joysticks and mouse)
    InputInterface::getInstance().updateResolution(w,h);
    //Update the graphical user interface
    vec2<U16> newResolution(w, h);
    GUI::getInstance().onResize(newResolution);
    //minimized
    _renderingPaused = (w == 0 || h == 0);
    APP.isFullScreen(!ParamHandler::getInstance().getParam<bool>("runtime.windowedMode"));
    if (APP.mainLoopActive()) {
        // Update light manager so that all shadow maps and other render targets match our needs
        LightManager::getInstance().updateResolution(w, h);
        // Cache resolution for faster access
        SceneManager::getInstance().cacheResolution(newResolution);
    }
}

///--------------------------Input Management-------------------------------------///

bool Kernel::setCursorPosition(U16 x, U16 y) const {
    _GFX.setCursorPosition(x, y);
    return true;
}

bool Kernel::onKeyDown(const OIS::KeyEvent& key) {
    if(!_sceneMgr.onKeyDown(key)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onKeyUp(const OIS::KeyEvent& key) {
    if(!_sceneMgr.onKeyUp(key)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onMouseMove(const OIS::MouseEvent& arg) {
    _cameraMgr->onMouseMove(arg);
    if(!_sceneMgr.onMouseMove(arg)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onMouseClickDown(const OIS::MouseEvent& arg, OIS::MouseButtonID button) {
    if(!_sceneMgr.onMouseClickDown(arg,button)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onMouseClickUp(const OIS::MouseEvent& arg, OIS::MouseButtonID button) {
    if(!_sceneMgr.onMouseClickUp(arg,button)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onJoystickMoveAxis(const OIS::JoyStickEvent& arg, I8 axis, I32 deadZone) {
    if(!_sceneMgr.onJoystickMoveAxis(arg,axis,deadZone)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onJoystickMovePOV(const OIS::JoyStickEvent& arg, I8 pov){
    if(!_sceneMgr.onJoystickMovePOV(arg,pov)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onJoystickButtonDown(const OIS::JoyStickEvent& arg, I8 button){
    if(!_sceneMgr.onJoystickButtonDown(arg,button)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
    if(!_sceneMgr.onJoystickButtonUp(arg,button)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    if(!_sceneMgr.sliderMoved(arg,index)) {
    }
    return true; //< InputInterface needs to know when this is completed
}

bool Kernel::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
    if(!_sceneMgr.vector3Moved(arg,index)) {
    }
    return true; //< InputInterface needs to know when this is completed
}