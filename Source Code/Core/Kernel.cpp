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
#include "Platform/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

U64 Kernel::_previousTime = 0ULL;
U64 Kernel::_currentTime = 0ULL;
U64 Kernel::_currentTimeFrozen = 0ULL;
U64 Kernel::_currentTimeDelta = 0ULL;
U64 Kernel::_nextGameTick = 0ULL;

bool Kernel::_keepAlive = true;
bool Kernel::_renderingPaused = false;
bool Kernel::_freezeLoopTime = false;
bool Kernel::_freezeGUITime = false;
Time::ProfileTimer* s_appLoopTimer = nullptr;

boost::lockfree::queue<I64> Kernel::_threadedCallbackBuffer(32);
Kernel::CallbackFunctions Kernel::_threadedCallbackFunctions;

Util::GraphPlot2D Kernel::_appTimeGraph("APP_TIME_GRAPH");

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
    ResourceCache::createInstance();
    FrameListenerManager::createInstance();
    // General light management and rendering (individual lights are handled by
    // each scene)
    // Unloading the lights is a scene level responsibility
    LightManager::createInstance();
    _cameraMgr.reset(new CameraManager(this));  // Camera manager
    assert(_cameraMgr != nullptr);
    // force all lights to update on camera change (to keep them still actually)
    _cameraMgr->addCameraUpdateListener(DELEGATE_BIND(
        &LightManager::onCameraUpdate, &LightManager::getInstance(), std::placeholders::_1));
    _cameraMgr->addCameraUpdateListener(
        DELEGATE_BIND(&Attorney::SceneManagerKernel::onCameraUpdate, std::placeholders::_1));
    // We have an A.I. thread, a networking thread, a PhysX thread, the main
    // update/rendering thread so how many threads do we allocate for tasks?
    // That's up to the programmer to decide for each app.
    // We add the A.I. thread in the same pool as it's a task. 
    // ReCast should also use this thread.
    _mainTaskPool.size_controller().resize(HARDWARE_THREAD_COUNT() + 1);

    ParamHandler::getInstance().setParam<stringImpl>("language", Locale::currentLanguage());

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
    Task_ptr taskPtr(MemoryManager_NEW Task(_mainTaskPool, tickInterval,
                                            numberOfTicks, threadedFunction));
    taskPtr->onCompletionCbk(DELEGATE_BIND(&Kernel::threadPoolCompleted, this,
                                           std::placeholders::_1));
    if (onCompletionFunction) {
        hashAlg::emplace(_threadedCallbackFunctions,
                taskPtr->getGUID(), onCompletionFunction);
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

    _freezeGUITime = par.getParam("freezeGUITime", false);
    bool freezeLoopTime = par.getParam("freezeLoopTime", false);
    if (freezeLoopTime != _freezeLoopTime) {
        _freezeLoopTime = freezeLoopTime;
        _currentTimeFrozen = _currentTime;
        Application::getInstance().mainLoopPaused(_freezeLoopTime);
    }

    const stringImpl& pendingLanguage = par.getParam<stringImpl>("language");
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
    if (!_keepAlive) {
        // exiting the rendering loop will return us to the last control point
        // (i.e. Kernel::runLogicLoop)
        Application::getInstance().mainLoopActive(false);
        return;
    }

    // Update internal timer
    Time::ApplicationTimer::getInstance().update(GFX_DEVICE.getFrameCount());

    Time::START_TIMER(*s_appLoopTimer);

    // Update time at every render loop
    _previousTime = _currentTime;
    _currentTime = Time::ElapsedMicroseconds();
    _currentTimeDelta = _currentTime - _previousTime;

    FrameEvent evt;
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();
    Application& APP = Application::getInstance();

    Kernel::idle();

    // Restore GPU to default state: clear buffers and set default render state
    GFX_DEVICE.beginFrame();
    {
        // Launch the FRAME_STARTED event
        frameMgr.createEvent(_currentTime, FrameEventType::FRAME_EVENT_STARTED, evt);
        _keepAlive = frameMgr.frameEvent(evt);

        // Process the current frame
        _keepAlive = APP.getKernel().mainLoopScene(evt) && _keepAlive;

        // Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended
        // event)
        frameMgr.createEvent(_currentTime, FrameEventType::FRAME_EVENT_PROCESS, evt);
        _keepAlive = frameMgr.frameEvent(evt) && _keepAlive;
    }
    GFX_DEVICE.endFrame();

    // Launch the FRAME_ENDED event (buffers have been swapped)
    frameMgr.createEvent(_currentTime, FrameEventType::FRAME_EVENT_ENDED, evt);
    _keepAlive = frameMgr.frameEvent(evt) && _keepAlive;

    _keepAlive = !APP.ShutdownRequested() && _keepAlive;
    ErrorCode err = APP.errorCode();
    if (err != ErrorCode::NO_ERR) {
        Console::errorfn("Error detected: [ %s ]", getErrorCodeName(err));
        _keepAlive = false;
    }
    Time::STOP_TIMER(*s_appLoopTimer);

#if defined(_DEBUG) || defined(_PROFILE)
    if (GFX_DEVICE.getFrameCount() % Config::TARGET_FRAME_RATE == 0) {
        Util::PlotFloatEvents("kernel.mainLoopApp", Util::GetFloatEvents(), _appTimeGraph);
    }
    if (_appTimeGraph._coords.size() % 2 == 0) {
        GFX::ScopedViewport viewport(0, 256, 256, 256);
        GFX_DEVICE.plot2DGraph(_appTimeGraph, vec4<U8>(255,0,0,255));
    }
    if (GFX_DEVICE.getFrameCount() % (Config::TARGET_FRAME_RATE * 10) == 0) {
        Console::printfn(
            "GPU: [ %5.5f ] [DrawCalls: %d]",
            Time::MicrosecondsToSeconds<F32>(GFX_DEVICE.getFrameDurationGPU()),
            GFX_DEVICE.getDrawCallCount());

        Util::FlushFloatEvents();
    }
    Util::RecordFloatEvent("kernel.mainLoopApp", s_appLoopTimer->get(), _currentTime);
#endif
}

bool Kernel::mainLoopScene(FrameEvent& evt) {
    _tasks.erase(std::remove_if(std::begin(_tasks), std::end(_tasks),
                                [](const Task_ptr& task)
                                    -> bool { return task->isFinished(); }),
                 std::end(_tasks));

    U64 deltaTime = Config::USE_FIXED_TIMESTEP ? Config::SKIP_TICKS : _currentTimeDelta;
    _cameraMgr->update(deltaTime);

    if (_renderingPaused) {
        idle();
        return true;
    }

    // Process physics
    _PFX.process(_freezeLoopTime ? 0ULL : _currentTimeDelta);

    U8 loops = 0;
    while (_currentTime > _nextGameTick && loops < Config::MAX_FRAMESKIP) {

        if (!_freezeGUITime) {
            _sceneMgr.processGUI(deltaTime);
        }

        if (!_freezeLoopTime) {
            // Update scene based on input
            _sceneMgr.processInput(deltaTime);
            // process all scene events
            _sceneMgr.processTasks(deltaTime);
            // Update the scene state based on current time (e.g. animation matrices)
            _sceneMgr.updateSceneState(deltaTime);
        }

        _nextGameTick += deltaTime;
        loops++;

        if (Config::USE_FIXED_TIMESTEP) {
            if (loops == Config::MAX_FRAMESKIP && _currentTime > _nextGameTick) {
                _nextGameTick = _currentTime;
            }
        } else {
            _nextGameTick = _currentTime;
        }

    }  // while

    D32 interpolationFactor = static_cast<D32>(_currentTime + deltaTime - _nextGameTick);
    interpolationFactor /= static_cast<D32>(deltaTime);
    assert(interpolationFactor <= 1.0);

    _GFX.setInterpolation(Config::USE_FIXED_TIMESTEP ? interpolationFactor : 1.0);
    
    // Get input events
    _APP.hasFocus() ? _input.update(_currentTimeDelta) : _sceneMgr.onLostFocus();
    // Update physics - uses own timestep implementation
    _PFX.update(_freezeLoopTime ? 0ULL : _currentTimeDelta);
    // Update the graphical user interface
    GUI::getInstance().update(_freezeGUITime ? 0ULL : _currentTimeDelta);

    return presentToScreen(evt);
}

void Kernel::renderScene() {
    //HACK: postprocessing not working with deffered rendering! -Ionut
    bool postProcessing = _GFX.getRenderer().getType() != RendererType::RENDERER_DEFERRED_SHADING &&
                          _GFX.postProcessingEnabled();

    if (_GFX.anaglyphEnabled() && postProcessing) {
        renderSceneAnaglyph();
    } else {

        Framebuffer::FramebufferTarget depthPassPolicy, colorPassPolicy;
        depthPassPolicy._drawMask =
            Framebuffer::FramebufferTarget::BufferMask::DEPTH;
        colorPassPolicy._drawMask =
            Framebuffer::FramebufferTarget::BufferMask::COLOR;

        // Z-prePass
        _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
            ->Begin(depthPassPolicy);
        _sceneMgr.render(RenderStage::Z_PRE_PASS, *this);
        _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->End();

        _GFX.ConstructHIZ();

        if (postProcessing) {
            _GFX.getRenderTarget(GFXDevice::RenderTarget::SCREEN)
                ->Begin(colorPassPolicy);
        }

        _sceneMgr.render(RenderStage::DISPLAY, *this);

        if (postProcessing) {
            _GFX.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->End();
            PostFX::getInstance().displayScene();
        }
    }
}

void Kernel::renderSceneAnaglyph() {
    Framebuffer::FramebufferTarget depthPassPolicy, colorPassPolicy;
    depthPassPolicy._drawMask =
        Framebuffer::FramebufferTarget::BufferMask::DEPTH;
    colorPassPolicy._drawMask =
        Framebuffer::FramebufferTarget::BufferMask::COLOR;
    Camera* currentCamera = _cameraMgr->getActiveCamera();

    // Render to right eye
    currentCamera->setAnaglyph(true);
    currentCamera->renderLookAt();
    // Z-prePass
    _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
        ->Begin(depthPassPolicy);
    SceneManager::getInstance().render(RenderStage::Z_PRE_PASS, *this);
    _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->End();
    // first screen buffer
    _GFX.getRenderTarget(GFXDevice::RenderTarget::SCREEN)
        ->Begin(colorPassPolicy);
    SceneManager::getInstance().render(RenderStage::DISPLAY, *this);
    _GFX.getRenderTarget(GFXDevice::RenderTarget::SCREEN)->End();

    // Render to left eye
    currentCamera->setAnaglyph(false);
    currentCamera->renderLookAt();
    // Z-prePass
    _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)
        ->Begin(depthPassPolicy);
    SceneManager::getInstance().render(RenderStage::Z_PRE_PASS, *this);
    _GFX.getRenderTarget(GFXDevice::RenderTarget::DEPTH)->End();
    // second screen buffer
    _GFX.getRenderTarget(GFXDevice::RenderTarget::ANAGLYPH)
        ->Begin(colorPassPolicy);
    SceneManager::getInstance().render(RenderStage::DISPLAY, *this);
    _GFX.getRenderTarget(GFXDevice::RenderTarget::ANAGLYPH)->End();

    PostFX::getInstance().displayScene();
}

bool Kernel::presentToScreen(FrameEvent& evt) {
    FrameListenerManager& frameMgr = FrameListenerManager::getInstance();

    frameMgr.createEvent(_currentTime, FrameEventType::FRAME_PRERENDER_START, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    // Prepare scene for rendering
    _sceneMgr.preRender();

    // perform time-sensitive shader tasks
    ShaderManager::getInstance().update(_currentTimeDelta);

    frameMgr.createEvent(_currentTime, FrameEventType::FRAME_PRERENDER_END, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    renderScene();

    frameMgr.createEvent(_currentTime, FrameEventType::FRAME_POSTRENDER_START, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    _sceneMgr.postRender();

    frameMgr.createEvent(_currentTime, FrameEventType::FRAME_POSTRENDER_END, evt);
    if (!frameMgr.frameEvent(evt)) {
        return false;
    }

    return true;
}

void Kernel::firstLoop() {
    ParamHandler& par = ParamHandler::getInstance();
    bool shadowMappingEnabled = par.getParam<bool>("rendering.enableShadows");
    // Skip two frames, one without and one with shadows, so all resources can
    // be built while
    // the splash screen is displayed
    par.setParam("freezeGUITime", true);
    par.setParam("freezeLoopTime", true);
    par.setParam("rendering.enableShadows", false);
    mainLoopApp();
    if (shadowMappingEnabled) {
        par.setParam("rendering.enableShadows", true);
        mainLoopApp();
    }
    GFX_DEVICE.setWindowPos(10, 60);
    par.setParam("freezeGUITime", false);
    par.setParam("freezeLoopTime", false);
    const vec2<U16> prevRes =
        Application::getInstance().getPreviousResolution();
    GFX_DEVICE.changeResolution(prevRes.width, prevRes.height);
#if defined(_DEBUG) || defined(_PROFILE)
    Time::ApplicationTimer::getInstance().benchmark(true);
#endif
    Attorney::SceneManagerKernel::initPostLoadState();

    _currentTime = _nextGameTick = Time::ElapsedMicroseconds();
}

void Kernel::submitRenderCall(RenderStage stage,
                              const SceneRenderState& sceneRenderState,
                              const DELEGATE_CBK<>& sceneRenderCallback) const {
    _GFX.setRenderStage(stage);
    _GFX.getRenderer().render(sceneRenderCallback, sceneRenderState);
}

ErrorCode Kernel::initialize(const stringImpl& entryPoint) {
    ParamHandler& par = ParamHandler::getInstance();

    Console::bindConsoleOutput(
        DELEGATE_BIND(&GUIConsole::printText, GUI::getInstance().getConsole(),
                      std::placeholders::_1, std::placeholders::_2));
    _PFX.setAPI(PXDevice::PhysicsAPI::PhysX);
    _SFX.setAPI(SFXDevice::AudioAPI::SDL);
    // Using OpenGL for rendering as default
    _GFX.setAPI(GFXDevice::RenderAPI::OpenGL);
    // Load info from XML files
    stringImpl startupScene(
        stringAlg::toBase(XML::loadScripts(stringAlg::fromBase(entryPoint))));
    // Create mem log file
    const stringImpl& mem = par.getParam<stringImpl>("memFile");
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get("START_RENDER_INTERFACE"));
    vec2<U16> resolution = _APP.getResolution();
    F32 aspectRatio = (F32)resolution.width / (F32)resolution.height;
    ErrorCode initError =
        _GFX.initRenderingAPI(vec2<U16>(400, 300), _argc, _argv);
    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    Console::printfn(Locale::get("SCENE_ADD_DEFAULT_CAMERA"));
    Camera* camera = MemoryManager_NEW FreeFlyCamera();
    camera->setProjection(aspectRatio,
                          par.getParam<F32>("rendering.verticalFOV"),
                          vec2<F32>(par.getParam<F32>("rendering.zNear"),
                                    par.getParam<F32>("rendering.zFar")));
    camera->setFixedYawAxis(true);
    // As soon as a camera is added to the camera manager, the manager is
    // responsible for cleaning it up
    _cameraMgr->addNewCamera("defaultCamera", camera);
    _cameraMgr->pushActiveCamera(camera);

    // Load and render the splash screen
    _GFX.setRenderStage(RenderStage::DISPLAY);
    _GFX.beginFrame();
    GUISplash("divideLogo.jpg", vec2<U16>(400, 300)).render();
    _GFX.endFrame();

    LightManager::getInstance().init();

    Console::printfn(Locale::get("START_SOUND_INTERFACE"));
    if ((initError = _SFX.initAudioAPI()) != ErrorCode::NO_ERR) {
        return initError;
    }

    Console::printfn(Locale::get("START_PHYSICS_INTERFACE"));
    if ((initError = _PFX.initPhysicsAPI(Config::TARGET_FRAME_RATE)) !=
        ErrorCode::NO_ERR) {
        return initError;
    }

    // Bind the kernel with the input interface
     if ((initError = Input::InputInterface::getInstance().init(*this)) !=
        ErrorCode::NO_ERR) {
        return initError;
    }
    // Initialize GUI with our current resolution
    _GUI.init(resolution);
    _sceneMgr.init(&_GUI);

    if (!_sceneMgr.load(startupScene, resolution)) {  //< Load the scene
        Console::errorfn(Locale::get("ERROR_SCENE_LOAD"), startupScene.c_str());
        return ErrorCode::MISSING_SCENE_DATA;
    }

    if (!_sceneMgr.checkLoadFlag()) {
        Console::errorfn(Locale::get("ERROR_SCENE_LOAD_NOT_CALLED"),
                         startupScene.c_str());
        return ErrorCode::MISSING_SCENE_LOAD_CALL;
    }

    camera->setMoveSpeedFactor(par.getParam<F32>("options.cameraSpeed.move"));
    camera->setTurnSpeedFactor(par.getParam<F32>("options.cameraSpeed.turn"));

    Console::printfn(Locale::get("INITIAL_DATA_LOADED"));
    Console::printfn(Locale::get("CREATE_AI_ENTITIES_START"));
    // Initialize and start the AI
    _sceneMgr.initializeAI(true);
    Console::printfn(Locale::get("CREATE_AI_ENTITIES_END"));

    return initError;
}

void Kernel::runLogicLoop() {
    Console::printfn(Locale::get("START_RENDER_LOOP"));
    Kernel::_nextGameTick = Time::ElapsedMicroseconds();
    // lock the scene
    GET_ACTIVE_SCENE()->state().runningState(true);
    // The first loops compiles all the visible data, so do not render the first
    // couple of frames
    firstLoop();

    _keepAlive = true;
    _APP.mainLoopActive(true);

    while (_APP.mainLoopActive()) {
        Kernel::mainLoopApp();
    }

    shutdown();
}

void Kernel::shutdown() {
    Console::printfn(Locale::get("STOP_KERNEL"));
    // release the scene
    GET_ACTIVE_SCENE()->state().runningState(false);
    Console::bindConsoleOutput(std::function<void(const char*, bool)>());
    GUI::destroyInstance();  /// Deactivate GUI
    _sceneMgr.unloadCurrentScene();
    _sceneMgr.deinitializeAI(true);
    SceneManager::destroyInstance();
    // Close CEGUI
    try {
        CEGUI::System::destroy();
    } catch (...) {
        Console::d_errorfn(Locale::get("ERROR_CEGUI_DESTROY"));
    }
    _cameraMgr.reset(nullptr);
    LightManager::destroyInstance();
    Console::printfn(Locale::get("STOP_ENGINE_OK"));
    Console::printfn(Locale::get("STOP_PHYSICS_INTERFACE"));
    _PFX.closePhysicsAPI();
    PXDevice::destroyInstance();
    Console::printfn(Locale::get("STOP_HARDWARE"));
    _SFX.closeAudioAPI();
    _GFX.closeRenderingAPI();
    _mainTaskPool.wait();
    Input::InputInterface::destroyInstance();
    SFXDevice::destroyInstance();
    GFXDevice::destroyInstance();
    ResourceCache::destroyInstance();
    // Destroy the shader manager AFTER the resource cache
    ShaderManager::destroyInstance();
    FrameListenerManager::destroyInstance();
        
    _mainTaskPool.clear();
    WAIT_FOR_CONDITION(_mainTaskPool.active() == 0);
    
    Time::REMOVE_TIMER(s_appLoopTimer);
}

void Kernel::updateResolutionCallback(I32 w, I32 h) {
    Application& APP = Application::getInstance();
    APP.setResolution(w, h);
    // Update internal resolution tracking (used for joysticks and mouse)
    Input::InputInterface::getInstance().updateResolution(w, h);
    // Update the graphical user interface
    vec2<U16> newResolution(w, h);
    GUI::getInstance().onResize(newResolution);
    // minimized
    _renderingPaused = (w == 0 || h == 0);
    APP.isFullScreen(
        !ParamHandler::getInstance().getParam<bool>("runtime.windowedMode"));
    if (APP.mainLoopActive()) {
        // Update light manager so that all shadow maps and other render targets
        // match our needs
        LightManager::getInstance().updateResolution(w, h);
        // Cache resolution for faster access
        SceneManager::getInstance().cacheResolution(newResolution);
    }
}

///--------------------------Input
/// Management-------------------------------------///

bool Kernel::setCursorPosition(U16 x, U16 y) const {
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