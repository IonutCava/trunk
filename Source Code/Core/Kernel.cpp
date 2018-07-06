#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ProfileTimer.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Physics/Headers/PXDevice.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Platform/Compute/Headers/OpenCLInterface.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

LoopTimingData::LoopTimingData() : _previousTime(0ULL),
                                   _currentTime(0ULL),
                                   _currentTimeFrozen(0ULL),
                                   _currentTimeDelta(0ULL),
                                   _nextGameTick(0ULL),
                                   _keepAlive(true),
                                   _freezeLoopTime(false),
                                   _freezeGUITime(false)
{
}


LoopTimingData Kernel::_timingData;
Time::ProfileTimer* s_appLoopTimer = nullptr;

boost::lockfree::queue<I64> Kernel::_threadedCallbackBuffer(128);
Kernel::CallbackFunctions Kernel::_threadedCallbackFunctions;

Kernel::Kernel(I32 argc, char** argv, Application& parentApp)
    : _argc(argc),
      _argv(argv),
      _APP(parentApp),
      _GFX(GFXDevice::getInstance()),                // Video
      _SFX(SFXDevice::getInstance()),                // Audio
      _PFX(PXDevice::getInstance()),                 // Physics
      _input(Input::InputInterface::getInstance()),  // Input
      _GUI(GUI::getInstance()),  // Graphical User Interface
      _sceneMgr(SceneManager::getInstance())  // Scene Manager

{
    _mainCamera = nullptr;

    ResourceCache::createInstance();
    FrameListenerManager::createInstance();
    // General light management and rendering (individual lights are handled by each scene)
    // Unloading the lights is a scene level responsibility
    LightManager::createInstance();
    OpenCLInterface::createInstance();
    _cameraMgr.reset(new CameraManager(this));  // Camera manager
    assert(_cameraMgr != nullptr);
    // force all lights to update on camera change (to keep them still actually)
    _cameraMgr->addCameraUpdateListener(DELEGATE_BIND(
        &LightManager::onCameraUpdate, &LightManager::getInstance(), std::placeholders::_1));
    _cameraMgr->addCameraUpdateListener(
        DELEGATE_BIND(&Attorney::SceneManagerKernel::onCameraUpdate, std::placeholders::_1));
    _cameraMgr->addCameraUpdateListener(
        DELEGATE_BIND(&Attorney::GFXDeviceKernel::onCameraUpdate, std::placeholders::_1));
    ParamHandler::getInstance().setParam<stringImpl>(_ID("language"), Locale::currentLanguage());

    // Add our needed app-wide render passes. RenderPassManager is responsible for deleting these!
    RenderPassManager::getInstance().addRenderPass("environmentPass", 0, {RenderStage::REFLECTION}).specialFlag(true);
    RenderPassManager::getInstance().addRenderPass("shadowPass",      1, {RenderStage::SHADOW});
    RenderPassManager::getInstance().addRenderPass("displayStage",    2, {RenderStage::Z_PRE_PASS, RenderStage::DISPLAY});

    s_appLoopTimer = Time::ADD_TIMER("MainLoopTimer");
}

Kernel::~Kernel()
{
    _mainTaskPool.clear();
}

void Kernel::threadPoolCompleted(I64 onExitTaskID) {
    WAIT_FOR_CONDITION(_threadedCallbackBuffer.push(onExitTaskID));
}

Task_ptr Kernel::AddTask(U64 tickInterval, I32 numberOfTicks,
                         const DELEGATE_CBK<>& threadedFunction,
                         const DELEGATE_CBK<>& onCompletionFunction) {
    Task_ptr taskPtr(new Task(_mainTaskPool,
                              tickInterval,
                              numberOfTicks,
                              threadedFunction));

    taskPtr->onCompletionCbk(DELEGATE_BIND(&Kernel::threadPoolCompleted,
                                           this,
                                           std::placeholders::_1));
    if (onCompletionFunction) {
        hashAlg::emplace(_threadedCallbackFunctions,
                         taskPtr->getGUID(),
                         onCompletionFunction);
    }

    _tasks.push_back(taskPtr);

    return taskPtr;
}

void Kernel::idle() {
    Console::flush();
    GFX_DEVICE.idle();
    PHYSICS_DEVICE.idle();
    SceneManager::getInstance().idle();
    LightManager::getInstance().idle();
    FrameListenerManager::getInstance().idle();

    ParamHandler& par = ParamHandler::getInstance();

    _timingData._freezeGUITime = par.getParam(_ID("freezeGUITime"), false);
    bool freezeLoopTime = par.getParam(_ID("freezeLoopTime"), false);
    if (freezeLoopTime != _timingData._freezeLoopTime) {
        _timingData._freezeLoopTime = freezeLoopTime;
        _timingData._currentTimeFrozen = _timingData._currentTime;
        Application::getInstance().mainLoopPaused(_timingData._freezeLoopTime);
    }

    const stringImpl& pendingLanguage = par.getParam<stringImpl>(_ID("language"));
    if (pendingLanguage.compare(Locale::currentLanguage()) != 0) {
        Locale::changeLanguage(pendingLanguage);
    }

    I64 taskGUID = -1;
    if (_threadedCallbackBuffer.pop(taskGUID)) {
        CallbackFunctions::const_iterator it = _threadedCallbackFunctions.find(taskGUID);
        assert(it != std::end(_threadedCallbackFunctions));
        if (it->second) {
            it->second();
        }
    }
}

void Kernel::mainLoopApp() {
    if (!_timingData._keepAlive) {
        // exiting the rendering loop will return us to the last control point
        // (i.e. Kernel::runLogicLoop)
        Application::getInstance().mainLoopActive(false);
        return;
    }

    // Update internal timer
    Time::ApplicationTimer::getInstance().update();

    Time::START_TIMER(*s_appLoopTimer);

    // Update time at every render loop
    _timingData._previousTime = _timingData._currentTime;
    _timingData._currentTime = Time::ElapsedMicroseconds();
    _timingData._currentTimeDelta = _timingData._currentTime - 
                                    _timingData._previousTime;

    // In case we break in the debugger
    if (_timingData._currentTimeDelta > Time::SecondsToMicroseconds(1)) {
        _timingData._currentTimeDelta = Config::SKIP_TICKS;
        _timingData._previousTime = _timingData._currentTime -
                                    _timingData._currentTimeDelta;
    }

    Kernel::idle();

    FrameEvent evt;
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
    Application& APP = Application::getInstance();

    // Restore GPU to default state: clear buffers and set default render state
    GFX_DEVICE.beginFrame();
    {
        // Launch the FRAME_STARTED event
        frameMgr.createEvent(_timingData._currentTime,
                             FrameEventType::FRAME_EVENT_STARTED, evt);
        _timingData._keepAlive = frameMgr.frameEvent(evt);

        // Process the current frame
        _timingData._keepAlive = APP.getKernel().mainLoopScene(evt) &&
                                 _timingData._keepAlive;

        // Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended
        // event)
        frameMgr.createEvent(_timingData._currentTime,
                             FrameEventType::FRAME_EVENT_PROCESS, evt);

        _timingData._keepAlive = frameMgr.frameEvent(evt) && 
                                 _timingData._keepAlive;
    }
    GFX_DEVICE.endFrame();

    // Launch the FRAME_ENDED event (buffers have been swapped)
    frameMgr.createEvent(_timingData._currentTime,
                         FrameEventType::FRAME_EVENT_ENDED, evt);
    _timingData._keepAlive = frameMgr.frameEvent(evt) &&
                             _timingData._keepAlive;

    _timingData._keepAlive = !APP.ShutdownRequested() &&
                             _timingData._keepAlive;

    ErrorCode err = APP.errorCode();
    if (err != ErrorCode::NO_ERR) {
        Console::errorfn("Error detected: [ %s ]", getErrorCodeName(err));
        _timingData._keepAlive = false;
    }
    Time::STOP_TIMER(*s_appLoopTimer);

#if !defined(_RELEASE)
    if (GFX_DEVICE.getFrameCount() % (Config::TARGET_FRAME_RATE * 10) == 0) {
        Console::printfn(
            "GPU: [ %5.5f ] [DrawCalls: %d]",
            Time::MicrosecondsToSeconds<F32>(GFX_DEVICE.getFrameDurationGPU()),
            GFX_DEVICE.getDrawCallCount());

        Util::FlushFloatEvents();
    }

    Util::RecordFloatEvent("kernel.mainLoopApp",
                           to_float(s_appLoopTimer->get()),
                           _timingData._currentTime);
#endif
}

bool Kernel::mainLoopScene(FrameEvent& evt) {
    _tasks.erase(std::remove_if(std::begin(_tasks), std::end(_tasks),
                                [](const Task_ptr& task)
                                    -> bool { return task->isFinished(); }),
                 std::end(_tasks));

    U64 deltaTime = Config::USE_FIXED_TIMESTEP ? Config::SKIP_TICKS 
                                               : _timingData._currentTimeDelta;
    // Update camera
    _cameraMgr->update(deltaTime);

    if (_APP.getWindowManager().minimized()) {
        idle();
        return true;
    }

    // Process physics
    _PFX.process(_timingData._freezeLoopTime ? 0ULL
                                             : _timingData._currentTimeDelta);

    U8 loops = 0;
    while (_timingData._currentTime > _timingData._nextGameTick &&
           loops < Config::MAX_FRAMESKIP) {

        if (!_timingData._freezeGUITime) {
            _sceneMgr.processGUI(deltaTime);
        }

        if (!_timingData._freezeLoopTime) {
            // Update scene based on input
            _sceneMgr.processInput(deltaTime);
            // process all scene events
            _sceneMgr.processTasks(deltaTime);
            // Update the scene state based on current time (e.g. animation matrices)
            _sceneMgr.updateSceneState(deltaTime);
        }

        _timingData._nextGameTick += deltaTime;
        loops++;

        if (Config::USE_FIXED_TIMESTEP) {
            if (loops == Config::MAX_FRAMESKIP &&
                _timingData._currentTime > _timingData._nextGameTick) {
                _timingData._nextGameTick = _timingData._currentTime;
            }
        } else {
            _timingData._nextGameTick = _timingData._currentTime;
        }

    }  // while

    D32 interpolationFactor = static_cast<D32>(_timingData._currentTime + 
                                               deltaTime -
                                               _timingData._nextGameTick);

    interpolationFactor /= static_cast<D32>(deltaTime);
    assert(interpolationFactor <= 1.0 && interpolationFactor > 0.0);

    _GFX.setInterpolation(Config::USE_FIXED_TIMESTEP ? interpolationFactor : 1.0);
    
    // Get input events
    if (_APP.getWindowManager().hasFocus()) {
        _input.update(_timingData._currentTimeDelta);
    } else {
        _sceneMgr.onLostFocus();
    }
    // Update physics - uses own timestep implementation
    _PFX.update(_timingData._freezeLoopTime ? 0ULL
                                            : _timingData._currentTimeDelta);
    // Update the graphical user interface
    if (!_timingData._freezeGUITime) {
        GUI::getInstance().update(_timingData._freezeGUITime ? 0ULL
                                                             : _timingData._currentTimeDelta);
    }

    return presentToScreen(evt);
}

bool Kernel::presentToScreen(FrameEvent& evt) {
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();

    frameMgr.createEvent(_timingData._currentTime,
                         FrameEventType::FRAME_PRERENDER_START, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    // perform time-sensitive shader tasks
    ShaderManager::getInstance().update(_timingData._currentTimeDelta);

    frameMgr.createEvent(_timingData._currentTime,
                         FrameEventType::FRAME_PRERENDER_END, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    RenderPassManager::getInstance().render(_sceneMgr.getActiveScene().renderState());
    if (_GFX.anaglyphEnabled()) {
        RenderPassManager::getInstance().render(_sceneMgr.getActiveScene().renderState(), true);
    }
    PostFX::getInstance().displayScene(_GFX.postProcessingEnabled());

    frameMgr.createEvent(_timingData._currentTime,
                         FrameEventType::FRAME_POSTRENDER_START, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    frameMgr.createEvent(_timingData._currentTime,
                         FrameEventType::FRAME_POSTRENDER_END, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    return true;
}

void Kernel::firstLoop() {
    ParamHandler& par = ParamHandler::getInstance();
    bool shadowMappingEnabled = par.getParam<bool>(_ID("rendering.enableShadows"));
    // Skip two frames, one without and one with shadows, so all resources can
    // be built while
    // the splash screen is displayed
    par.setParam(_ID("freezeGUITime"), true);
    par.setParam(_ID("freezeLoopTime"), true);
    par.setParam(_ID("rendering.enableShadows"), false);
    mainLoopApp();
    if (shadowMappingEnabled) {
        par.setParam(_ID("rendering.enableShadows"), true);
        mainLoopApp();
    }
    mainLoopApp();
    par.setParam(_ID("freezeGUITime"), false);
    par.setParam(_ID("freezeLoopTime"), false);
#if defined(_DEBUG) || defined(_PROFILE)
    Time::ApplicationTimer::getInstance().benchmark(true);
#endif
    Attorney::SceneManagerKernel::initPostLoadState();

    _timingData._currentTime = _timingData._nextGameTick = Time::ElapsedMicroseconds();
}

ErrorCode Kernel::initialize(const stringImpl& entryPoint) {
    ParamHandler& par = ParamHandler::getInstance();

    // We have an A.I. thread, a networking thread, a PhysX thread, the main
    // update/rendering thread so how many threads do we allocate for tasks?
    // That's up to the programmer to decide for each app.
    // We add the A.I. thread in the same pool as it's a task. 
    // ReCast should also use this thread.
    U32 threadCount = HARDWARE_THREAD_COUNT();
    if (threadCount <= 2) {
        return ErrorCode::CPU_NOT_SUPPORTED;
    }

    SysInfo& systemInfo = Application::getInstance().getSysInfo();
    if (!CheckMemory(Config::REQUIRED_RAM_SIZE, systemInfo)) {
        return ErrorCode::NOT_ENOUGH_RAM;
    }

    _mainTaskPool.size_controller().resize(HARDWARE_THREAD_COUNT());

    Console::bindConsoleOutput(
        DELEGATE_BIND(&GUIConsole::printText, GUI::getInstance().getConsole(),
                      std::placeholders::_1, std::placeholders::_2));
    _PFX.setAPI(PXDevice::PhysicsAPI::PhysX);
    _SFX.setAPI(SFXDevice::AudioAPI::SDL);
    // Using OpenGL for rendering as default
    if (Config::USE_OPENGL_RENDERING) {
        if (Config::USE_OPENGL_ES) {
            _GFX.setAPI(GFXDevice::RenderAPI::OpenGLES);
        } else {
            _GFX.setAPI(GFXDevice::RenderAPI::OpenGL);
        }
    }
    // Load info from XML files
    stringImpl startupScene(XML::loadScripts(entryPoint));
    // Create mem log file
    const stringImpl& mem = par.getParam<stringImpl>("memFile");
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get(_ID("START_RENDER_INTERFACE")));
    WindowManager& winManager = _APP.getWindowManager();
    WindowType windowType = winManager.mainWindowType();

    vec2<U16> resolution = winManager.getResolution();
    F32 aspectRatio = to_float(resolution.width) / to_float(resolution.height);

    ErrorCode initError = _GFX.initRenderingAPI(_argc, _argv);
    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }
    initError = OpenCLInterface::getInstance().init();
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }
    Console::printfn(Locale::get(_ID("SCENE_ADD_DEFAULT_CAMERA")));
    _mainCamera = MemoryManager_NEW FreeFlyCamera();
    _mainCamera->setProjection(aspectRatio,
                               par.getParam<F32>("rendering.verticalFOV"),
                               vec2<F32>(par.getParam<F32>("rendering.zNear"),
                                         par.getParam<F32>("rendering.zFar")));
    _mainCamera->setFixedYawAxis(true);
    // As soon as a camera is added to the camera manager, the manager is
    // responsible for cleaning it up
    _cameraMgr->addNewCamera("defaultCamera", _mainCamera);
    _cameraMgr->pushActiveCamera(_mainCamera);
    // We start of with a forward plus renderer
    _sceneMgr.setRenderer(RendererType::RENDERER_FORWARD_PLUS);
    winManager.mainWindowType(WindowType::SPLASH);
    // Load and render the splash screen
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg", winManager.getWindowDimensions(WindowType::SPLASH)).render();
    _GFX.endFrame();

    winManager.mainWindowType(windowType);
    Console::printfn(Locale::get(_ID("START_SOUND_INTERFACE")));
    if ((initError = _SFX.initAudioAPI()) != ErrorCode::NO_ERR) {
        return initError;
    }

    Console::printfn(Locale::get(_ID("START_PHYSICS_INTERFACE")));
    if ((initError = _PFX.initPhysicsAPI(Config::TARGET_FRAME_RATE)) !=
        ErrorCode::NO_ERR) {
        return initError;
    }

    // Bind the kernel with the input interface
     if ((initError = Input::InputInterface::getInstance().init(*this)) !=
        ErrorCode::NO_ERR) {
        return initError;
    }

    LightManager::getInstance().init();

    // Initialize GUI with our current resolution
    _GUI.init(resolution);
    _sceneMgr.init(&_GUI);

    if (!_sceneMgr.load(startupScene, resolution)) {  //< Load the scene
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")), startupScene.c_str());
        return ErrorCode::MISSING_SCENE_DATA;
    }

    if (!_sceneMgr.checkLoadFlag()) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_NOT_CALLED")),
                         startupScene.c_str());
        return ErrorCode::MISSING_SCENE_LOAD_CALL;
    }

    _mainCamera->setMoveSpeedFactor(par.getParam<F32>("options.cameraSpeed.move"));
    _mainCamera->setTurnSpeedFactor(par.getParam<F32>("options.cameraSpeed.turn"));

    Console::printfn(Locale::get(_ID("INITIAL_DATA_LOADED")));
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_START")));
    // Initialize and start the AI
    _sceneMgr.initializeAI(true);
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_END")));

    return initError;
}

void Kernel::runLogicLoop() {
    Console::printfn(Locale::get(_ID("START_RENDER_LOOP")));
    _timingData._nextGameTick = Time::ElapsedMicroseconds();
    // lock the scene
    GET_ACTIVE_SCENE().state().runningState(true);
    // The first loops compiles all the visible data, so do not render the first
    // couple of frames
    firstLoop();

    _timingData._keepAlive = true;
    _APP.mainLoopActive(true);
    Console::printfn(Locale::get(_ID("START_MAIN_LOOP")));
    while (_APP.mainLoopActive()) {
        mainLoopApp();
    }

    shutdown();
}

void Kernel::shutdown() {
    Console::printfn(Locale::get(_ID("STOP_KERNEL")));
    _mainTaskPool.clear();
    WAIT_FOR_CONDITION(_mainTaskPool.active() == 0);

    // release the scene
    GET_ACTIVE_SCENE().state().runningState(false);
    Console::bindConsoleOutput(std::function<void(const char*, bool)>());
    GUI::destroyInstance();  /// Deactivate GUI
    _sceneMgr.unloadCurrentScene();
    SceneManager::destroyInstance();
    // Close CEGUI
    try {
        CEGUI::System::destroy();
    } catch (...) {
        Console::d_errorfn(Locale::get(_ID("ERROR_CEGUI_DESTROY")));
    }
    _cameraMgr.reset(nullptr);
    LightManager::destroyInstance();
    Console::printfn(Locale::get(_ID("STOP_ENGINE_OK")));
    Console::printfn(Locale::get(_ID("STOP_PHYSICS_INTERFACE")));
    _PFX.closePhysicsAPI();
    PXDevice::destroyInstance();
    Console::printfn(Locale::get(_ID("STOP_HARDWARE")));
    OpenCLInterface::getInstance().deinit();
    _SFX.closeAudioAPI();
    _GFX.closeRenderingAPI();
    Input::InputInterface::destroyInstance();
    SFXDevice::destroyInstance();
    GFXDevice::destroyInstance();
    ResourceCache::destroyInstance();
    // Destroy the shader manager AFTER the resource cache
    ShaderManager::destroyInstance();
    FrameListenerManager::destroyInstance();
        
   
    Time::REMOVE_TIMER(s_appLoopTimer);
}

void Kernel::onChangeWindowSize(U16 w, U16 h) {
    if (Input::InputInterface::getInstance().isInit()) {
        const OIS::MouseState& ms = Input::InputInterface::getInstance().getMouse().getMouseState();
        ms.width = w;
        ms.height = h;
    }

    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(w, h));

    if (_mainCamera) {
        _mainCamera->setAspectRatio(to_float(w) / to_float(h));
    }
}

///--------------------------Input
/// Management-------------------------------------///

bool Kernel::setCursorPosition(I32 x, I32 y) const {
    _GFX.setCursorPosition(x, y);
    _GUI.setCursorPosition(x, y);
    return true;
}

bool Kernel::onKeyDown(const Input::KeyEvent& key) {
    if (_GUI.onKeyDown(key)) {
        return _sceneMgr.onKeyDown(key);
    }
    return true;  //< InputInterface needs to know when this is completed
}

bool Kernel::onKeyUp(const Input::KeyEvent& key) {
    if (_GUI.onKeyUp(key)) {
        return _sceneMgr.onKeyUp(key);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseMoved(const Input::MouseEvent& arg) {
    _cameraMgr->mouseMoved(arg);
    if (_GUI.mouseMoved(arg)) {
        return _sceneMgr.mouseMoved(arg);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonPressed(const Input::MouseEvent& arg,
                                Input::MouseButton button) {
    if (_GUI.mouseButtonPressed(arg, button)) {
        return _sceneMgr.mouseButtonPressed(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonReleased(const Input::MouseEvent& arg,
                                 Input::MouseButton button) {
    if (_GUI.mouseButtonReleased(arg, button)) {
        return _sceneMgr.mouseButtonReleased(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (_GUI.joystickAxisMoved(arg, axis)) {
        return _sceneMgr.joystickAxisMoved(arg, axis);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (_GUI.joystickPovMoved(arg, pov)) {
        return _sceneMgr.joystickPovMoved(arg, pov);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonPressed(const Input::JoystickEvent& arg,
                                   Input::JoystickButton button) {
    if (_GUI.joystickButtonPressed(arg, button)) {
        return _sceneMgr.joystickButtonPressed(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonReleased(const Input::JoystickEvent& arg,
                                    Input::JoystickButton button) {
    if (_GUI.joystickButtonReleased(arg, button)) {
        return _sceneMgr.joystickButtonReleased(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    if (_GUI.joystickSliderMoved(arg, index)) {
        return _sceneMgr.joystickSliderMoved(arg, index);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index) {
    if (_GUI.joystickVector3DMoved(arg, index)) {
        return _sceneMgr.joystickVector3DMoved(arg, index);
    }
    // InputInterface needs to know when this is completed
    return false;
}
};
