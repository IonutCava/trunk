#include "Headers/Kernel.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Time/Headers/ApplicationTimer.h"
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

LoopTimingData::LoopTimingData() : _updateLoops(0),
                                   _previousTime(0ULL),
                                   _currentTime(0ULL),
                                   _currentTimeFrozen(0ULL),
                                   _currentTimeDelta(0ULL),
                                   _nextGameTick(0ULL),
                                   _keepAlive(true),
                                   _freezeLoopTime(false),
                                   _freezeGUITime(false)
{
}

bool Kernel::initStaticData() {
    return SceneManager::initStaticData();
}

Kernel::Kernel(I32 argc, char** argv, Application& parentApp)
    : _argc(argc),
      _argv(argv),
      _APP(parentApp),
      _taskPool(Config::MAX_POOLED_TASKS),
      _GFX(GFXDevice::instance()),                // Video
      _SFX(SFXDevice::instance()),                // Audio
      _PFX(PXDevice::instance()),                 // Physics
      _input(Input::InputInterface::instance()),  // Input
      _GUI(GUI::instance()),                      // Graphical User Interface
      _sceneMgr(SceneManager::instance()),        // Scene Manager
      _appLoopTimer(Time::ADD_TIMER("Main Loop Timer")),
      _frameTimer(Time::ADD_TIMER("Total Frame Timer")),
      _appIdleTimer(Time::ADD_TIMER("Loop Idle Timer")),
      _appScenePass(Time::ADD_TIMER("Loop Scene Pass Timer")),
      _physicsUpdateTimer(Time::ADD_TIMER("Physics Update Timer")),
      _sceneUpdateTimer(Time::ADD_TIMER("Scene Update Timer")),
      _sceneUpdateLoopTimer(Time::ADD_TIMER("Scene Update Loop timer")),
      _physicsProcessTimer(Time::ADD_TIMER("Physics Process Timer")),
      _cameraMgrTimer(Time::ADD_TIMER("Camera Manager Update Timer")),
      _flushToScreenTimer(Time::ADD_TIMER("Flush To Screen Timer"))
{

    _appLoopTimer.addChildTimer(_appIdleTimer);
    _appLoopTimer.addChildTimer(_frameTimer);
    _frameTimer.addChildTimer(_appScenePass);
    _appScenePass.addChildTimer(_cameraMgrTimer);
    _appScenePass.addChildTimer(_physicsUpdateTimer);
    _appScenePass.addChildTimer(_physicsProcessTimer);
    _appScenePass.addChildTimer(_sceneUpdateTimer);
    _appScenePass.addChildTimer(_flushToScreenTimer);
    _sceneUpdateTimer.addChildTimer(_sceneUpdateLoopTimer);

    ResourceCache::createInstance();
    FrameListenerManager::createInstance();
    OpenCLInterface::createInstance();
    _cameraMgr.reset(new CameraManager(this));  // Camera manager
    assert(_cameraMgr != nullptr);
    // force all lights to update on camera change (to keep them still actually)
    _cameraMgr->addCameraUpdateListener(
        DELEGATE_BIND(&Attorney::GFXDeviceKernel::onCameraUpdate, std::placeholders::_1));
    _cameraMgr->addCameraUpdateListener(
        DELEGATE_BIND(&Attorney::SceneManagerKernel::onCameraUpdate, std::placeholders::_1));
    ParamHandler::instance().setParam<stringImpl>(_ID("language"), Locale::currentLanguage());
}

Kernel::~Kernel()
{
}

void Kernel::idle() {
    _GFX.idle();
    _PFX.idle();
    _sceneMgr.idle();
    FrameListenerManager::instance().idle();

    ParamHandler& par = ParamHandler::instance();

    _timingData._freezeGUITime = par.getParam(_ID("freezeGUITime"), false);
    bool freezeLoopTime = par.getParam(_ID("freezeLoopTime"), false);
    if (freezeLoopTime != _timingData._freezeLoopTime) {
        _timingData._freezeLoopTime = freezeLoopTime;
        _timingData._currentTimeFrozen = _timingData._currentTime;
        _APP.mainLoopPaused(_timingData._freezeLoopTime);
    }

    const stringImpl& pendingLanguage = par.getParam<stringImpl>(_ID("language"));
    if (pendingLanguage.compare(Locale::currentLanguage()) != 0) {
        Locale::changeLanguage(pendingLanguage);
    }

    _taskPool.flushCallbackQueue();
}

void Kernel::onLoop() {
    if (!_timingData._keepAlive) {
        // exiting the rendering loop will return us to the last control point
        _APP.mainLoopActive(false);
        return;
    }

    // Update internal timer
    Time::ApplicationTimer::instance().update();
    {
        Time::ScopedTimer timer(_appLoopTimer);
   
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

        {
            Time::ScopedTimer timer2(_appIdleTimer);
            idle();
        }
        FrameEvent evt;
        FrameListenerManager& frameMgr = FrameListenerManager::instance();

        // Restore GPU to default state: clear buffers and set default render state
        _GFX.beginFrame();
        {
            Time::ScopedTimer timer3(_frameTimer);
            // Launch the FRAME_STARTED event
            frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_EVENT_STARTED, evt);
            _timingData._keepAlive = frameMgr.frameEvent(evt);

            // Process the current frame
            _timingData._keepAlive = mainLoopScene(evt) && _timingData._keepAlive;

            // Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended
            // event)
            frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_EVENT_PROCESS, evt);

            _timingData._keepAlive = frameMgr.frameEvent(evt) && _timingData._keepAlive;
        }
        _GFX.endFrame(_APP.mainLoopActive());

        // Launch the FRAME_ENDED event (buffers have been swapped)
        frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_EVENT_ENDED, evt);
        _timingData._keepAlive = frameMgr.frameEvent(evt) && _timingData._keepAlive;

        _timingData._keepAlive = !_APP.ShutdownRequested() && _timingData._keepAlive;

        ErrorCode err = _APP.errorCode();

        if (err != ErrorCode::NO_ERR) {
            Console::errorfn("Error detected: [ %s ]", getErrorCodeName(err));
            _timingData._keepAlive = false;
        }
    }

if (Config::Profile::BENCHMARK_PERFORMANCE ||
    Config::Profile::ENABLE_FUNCTION_PROFILING)
{
    U32 frameCount = _GFX.getFrameCount();
    if (frameCount % 5 == 0) {
        stringImpl profileData(Util::StringFormat("Scene Update Loops: %d", _timingData._updateLoops));

        if (Config::Profile::BENCHMARK_PERFORMANCE) {
            profileData.append("\n");
            profileData.append(Time::ApplicationTimer::instance().benchmarkReport());
            profileData.append("\n");
            profileData.append(Util::StringFormat("GPU: [ %5.5f ] [DrawCalls: %d]",
                                                  Time::MicrosecondsToSeconds<F32>(_GFX.getFrameDurationGPU()),
                                                  _GFX.getDrawCallCount()));
        }
        if (Config::Profile::ENABLE_FUNCTION_PROFILING) {
            profileData.append("\n");
            profileData.append(Time::ProfileTimer::printAll());
        }
        Console::printfn(profileData.c_str());

        _GUI.modifyGlobalText(_ID("ProfileData"), profileData);
    }

    Util::RecordFloatEvent("kernel.mainLoopApp",
                           to_float(_appLoopTimer.get()),
                           _timingData._currentTime);

    if (frameCount % (Config::TARGET_FRAME_RATE * 10) == 0) {
        Util::FlushFloatEvents();
    }
}
}

bool Kernel::mainLoopScene(FrameEvent& evt) {
    Time::ScopedTimer timer(_appScenePass);

    U64 deltaTime = Config::USE_FIXED_TIMESTEP ? Config::SKIP_TICKS 
                                               : _timingData._currentTimeDelta;
    {
        Time::ScopedTimer timer2(_cameraMgrTimer);
        // Update cameras
        _cameraMgr->update(deltaTime);
    }

    if (_APP.windowManager().getActiveWindow().minimized()) {
        idle();
        return true;
    }

    {
        Time::ScopedTimer timer2(_physicsProcessTimer);
        // Process physics
        _PFX.process(_timingData._freezeLoopTime ? 0ULL
                                                : _timingData._currentTimeDelta);
    }

    {
        Time::ScopedTimer timer2(_sceneUpdateTimer);

        _timingData._updateLoops = 0;
        while (_timingData._currentTime > _timingData._nextGameTick &&
            _timingData._updateLoops < Config::MAX_FRAMESKIP) {

            if (_timingData._updateLoops == 0) {
                _sceneUpdateLoopTimer.start();
            }

            if (!_timingData._freezeGUITime) {
                _sceneMgr.processGUI(deltaTime);
            }

            if (!_timingData._freezeLoopTime) {
                // Flush any pending threaded callbacks
                _taskPool.flushCallbackQueue();
                // Update scene based on input
                _sceneMgr.processInput(deltaTime);
                // process all scene events
                _sceneMgr.processTasks(deltaTime);
                // Update the scene state based on current time (e.g. animation matrices)
                _sceneMgr.updateSceneState(deltaTime);
            }

            if (_timingData._updateLoops == 0) {
                _sceneUpdateLoopTimer.stop();
            }

            _timingData._nextGameTick += deltaTime;
            _timingData._updateLoops++;

            if (Config::USE_FIXED_TIMESTEP) {
                if (_timingData._updateLoops == Config::MAX_FRAMESKIP &&
                    _timingData._currentTime > _timingData._nextGameTick) {
                    _timingData._nextGameTick = _timingData._currentTime;
                }
            } else {
                _timingData._nextGameTick = _timingData._currentTime;
            }

        }  // while
    }
    D64 interpolationFactor = static_cast<D64>(_timingData._currentTime + 
                                               deltaTime -
                                               _timingData._nextGameTick);

    interpolationFactor /= static_cast<D64>(deltaTime);
    assert(interpolationFactor <= 1.0 && interpolationFactor > 0.0);

    _GFX.setInterpolation(Config::USE_FIXED_TIMESTEP ? interpolationFactor : 1.0);
    
    // Get input events
    if (_APP.windowManager().getActiveWindow().hasFocus()) {
        _input.update(_timingData._currentTimeDelta);
    } else {
        _sceneMgr.onLostFocus();
    }
    {
        Time::ScopedTimer timer3(_physicsUpdateTimer);
        // Update physics - uses own timestep implementation
        _PFX.update(_timingData._freezeLoopTime ? 0ULL
                                                : _timingData._currentTimeDelta);
    }
    // Update the graphical user interface
    if (!_timingData._freezeGUITime) {
        _GUI.update(_timingData._freezeGUITime ? 0ULL
                                               : _timingData._currentTimeDelta);
    }

    return presentToScreen(evt);
}

bool Kernel::presentToScreen(FrameEvent& evt) {
    Time::ScopedTimer time(_flushToScreenTimer);
    FrameListenerManager& frameMgr = FrameListenerManager::instance();

    frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_PRERENDER_START, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    // perform time-sensitive shader tasks
    ShaderManager::instance().update(_timingData._currentTimeDelta);

    frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_PRERENDER_END, evt);

    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    RenderPassManager::instance().render(_sceneMgr.getActiveScene().renderState());
    PostFX::instance().apply();

    Attorney::GFXDeviceKernel::flushDisplay();

    frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_POSTRENDER_START, evt);

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

// The first loops compiles all the visible data, so do not render the first couple of frames
void Kernel::warmup() {
    static const U8 warmupLoopCount = 3;

    Console::printfn(Locale::get(_ID("START_RENDER_LOOP")));
    _timingData._nextGameTick = Time::ElapsedMicroseconds();
    _timingData._keepAlive = true;

    ParamHandler& par = ParamHandler::instance();
    bool shadowMappingEnabled = par.getParam<bool>(_ID("rendering.enableShadows"));
    // Skip two frames, one without and one with shadows, so all resources can
    // be built while
    // the splash screen is displayed
    par.setParam(_ID("freezeGUITime"), true);
    par.setParam(_ID("freezeLoopTime"), true);
    par.setParam(_ID("rendering.enableShadows"), false);

    onLoop();
    if (shadowMappingEnabled) {
        par.setParam(_ID("rendering.enableShadows"), true);
        onLoop();
    }

    for (U8 i = 0; i < std::max(warmupLoopCount - 3, 0); ++i) {
        onLoop();
    }

    par.setParam(_ID("freezeGUITime"), false);
    par.setParam(_ID("freezeLoopTime"), false);
    Attorney::SceneManagerKernel::initPostLoadState();

    _timingData._currentTime = _timingData._nextGameTick = Time::ElapsedMicroseconds();
}

ErrorCode Kernel::initialize(const stringImpl& entryPoint) {
    ParamHandler& par = ParamHandler::instance();

    SysInfo& systemInfo = Application::instance().sysInfo();
    if (!CheckMemory(Config::REQUIRED_RAM_SIZE, systemInfo)) {
        return ErrorCode::NOT_ENOUGH_RAM;
    }

    // We have an A.I. thread, a networking thread, a PhysX thread, the main
    // update/rendering thread so how many threads do we allocate for tasks?
    // That's up to the programmer to decide for each app.
    if (!_taskPool.init(HARDWARE_THREAD_COUNT())) {
        return ErrorCode::CPU_NOT_SUPPORTED;
    }

    Console::bindConsoleOutput(
        DELEGATE_BIND(&GUIConsole::printText, _GUI.getConsole(),
                      std::placeholders::_1, std::placeholders::_2));
    _PFX.setAPI(PXDevice::PhysicsAPI::PhysX);
    _SFX.setAPI(SFXDevice::AudioAPI::SDL);
    // Using OpenGL for rendering as default
    if (Config::USE_OPENGL_RENDERING) {
        _GFX.setAPI(Config::USE_OPENGL_ES ? RenderAPI::OpenGLES
                                          : RenderAPI::OpenGL);
    }
    // Load info from XML files
    stringImpl startupScene(XML::loadScripts(entryPoint));
    // Create mem log file
    const stringImpl& mem = par.getParam<stringImpl>(_ID("memFile"));
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get(_ID("START_RENDER_INTERFACE")));

    // Fulscreen is automatically calculated
    ResolutionByType initRes;
    initRes[to_const_uint(WindowType::WINDOW)].set(par.getParam<I32>(_ID("runtime.windowWidth"), 1024),
                                                   par.getParam<I32>(_ID("runtime.windowHeight"), 768));
    initRes[to_const_uint(WindowType::SPLASH)].set(par.getParam<I32>(_ID("runtime.splashWidth"), 400),
                                                   par.getParam<I32>(_ID("runtime.splashHeight"), 300));

    I32 targetDisplay = par.getParam<I32>(_ID("runtime.targetDisplay"), 0);
    bool startFullScreen = par.getParam<bool>(_ID("runtime.startFullScreen"), true);
    WindowManager& winManager = _APP.windowManager();

    ErrorCode initError = winManager.init(_GFX.getAPI(), initRes, startFullScreen, targetDisplay);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Match initial rendering resolution to window/screen size
    const DisplayWindow& mainWindow  = winManager.getActiveWindow();
    vec2<U16> renderResolution(startFullScreen ? mainWindow.getDimensions(WindowType::FULLSCREEN)
                                               : mainWindow.getDimensions(WindowType::WINDOW));
    initError = _GFX.initRenderingAPI(_argc, _argv, renderResolution);

    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }
    initError = OpenCLInterface::instance().init();
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Add our needed app-wide render passes. RenderPassManager is responsible for deleting these!
    RenderPassManager::instance().addRenderPass("environmentPass", 0, { RenderStage::REFLECTION });
    RenderPassManager::instance().addRenderPass("shadowPass", 1, { RenderStage::SHADOW });
    RenderPassManager::instance().addRenderPass("displayStage", 2, { RenderStage::Z_PRE_PASS, RenderStage::DISPLAY });

    Console::printfn(Locale::get(_ID("SCENE_ADD_DEFAULT_CAMERA")));

    // As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
    _cameraMgr->pushActiveCamera(_cameraMgr->createCamera("defaultCamera", Camera::CameraType::FREE_FLY));
    _cameraMgr->getActiveCamera().setFixedYawAxis(true);
    _cameraMgr->getActiveCamera().setProjection(to_float(renderResolution.width) / to_float(renderResolution.height),
                                                par.getParam<F32>(_ID("rendering.verticalFOV")),
                                                vec2<F32>(par.getParam<F32>(_ID("rendering.zNear")),
                                                          par.getParam<F32>(_ID("rendering.zFar"))));
    // We start of with a forward plus renderer
    _sceneMgr.setRenderer(RendererType::RENDERER_FORWARD_PLUS);

    DisplayWindow& window = winManager.getActiveWindow();
    window.type(WindowType::SPLASH);
    winManager.handleWindowEvent(WindowEvent::APP_LOOP, -1, -1, -1);
    // Load and render the splash screen
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg", initRes[to_const_uint(WindowType::SPLASH)]).render();
    _GFX.endFrame(true);

    Console::printfn(Locale::get(_ID("START_SOUND_INTERFACE")));
    initError = _SFX.initAudioAPI();
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    Console::printfn(Locale::get(_ID("START_PHYSICS_INTERFACE")));
    initError = _PFX.initPhysicsAPI(Config::TARGET_FRAME_RATE);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Bind the kernel with the input interface
    initError = _input.init(*this, renderResolution);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Initialize GUI with our current resolution
    _GUI.init(renderResolution);
    _GUI.addGlobalText(_ID("ProfileData"),              // Unique ID
        vec2<I32>(renderResolution.width * 0.75, 100),  // Position
        Font::DROID_SERIF_BOLD,          // Font
        vec4<U8>(255,  50, 0, 255),      // Color
        "PROFILE DATA",                  // Text
        12);                             // Font size

    _sceneMgr.init(&_GUI);

    Scene* loadedScene = _sceneMgr.load(startupScene);
    if (loadedScene == nullptr) {  //< Load the scene
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")), startupScene.c_str());
        return ErrorCode::MISSING_SCENE_DATA;
    } else {
        _sceneMgr.setActiveScene(*loadedScene);
    }

    if (!_sceneMgr.checkLoadFlag()) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_NOT_CALLED")),
                         startupScene.c_str());
        return ErrorCode::MISSING_SCENE_LOAD_CALL;
    }

    Console::printfn(Locale::get(_ID("INITIAL_DATA_LOADED")));

    return initError;
}

void Kernel::shutdown() {
    Console::printfn(Locale::get(_ID("STOP_KERNEL")));

    // release the scene
    _sceneMgr.unloadCurrentScene();
    Console::bindConsoleOutput(std::function<void(const char*, bool)>());
    SceneManager::destroyInstance();
    GUI::destroyInstance();  /// Deactivate GUI
    _cameraMgr.reset(nullptr);
    ShadowMap::clearShadowMaps();
    Console::printfn(Locale::get(_ID("STOP_ENGINE_OK")));
    Console::printfn(Locale::get(_ID("STOP_PHYSICS_INTERFACE")));
    _PFX.closePhysicsAPI();
    PXDevice::destroyInstance();
    Console::printfn(Locale::get(_ID("STOP_HARDWARE")));
    OpenCLInterface::instance().deinit();
    _SFX.closeAudioAPI();
    _GFX.closeRenderingAPI();
    Input::InputInterface::destroyInstance();
    SFXDevice::destroyInstance();
    GFXDevice::destroyInstance();
    ResourceCache::destroyInstance();
    // Destroy the shader manager AFTER the resource cache
    ShaderManager::destroyInstance();
    FrameListenerManager::destroyInstance();
}

void Kernel::onChangeWindowSize(U16 w, U16 h) {
    Attorney::GFXDeviceKernel::onChangeWindowSize(w, h);

    if (_input.isInit()) {
        const OIS::MouseState& ms = _input.getMouse().getMouseState();
        ms.width = w;
        ms.height = h;
    }

    _GUI.onChangeResolution(w, h);
}

void Kernel::onChangeRenderResolution(U16 w, U16 h) const {
    Attorney::GFXDeviceKernel::onChangeRenderResolution(w, h);
    _GUI.onChangeResolution(w, h);


    Camera* mainCamera = _cameraMgr->findCamera(_ID_RT("defaultCamera"));
    if (mainCamera) {
        const ParamHandler& par = ParamHandler::instance();
        mainCamera->setProjection(to_float(w) / to_float(h),
                                  par.getParam<F32>(_ID("rendering.verticalFOV")),
                                  vec2<F32>(par.getParam<F32>(_ID("rendering.zNear")),
                                            par.getParam<F32>(_ID("rendering.zFar"))));
    }
}

///--------------------------Input Management-------------------------------------///
bool Kernel::setCursorPosition(I32 x, I32 y) const {
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
