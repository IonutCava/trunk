#include "stdafx.h"

#include "config.h"

#include "Headers/Kernel.h"
#include "Headers/XMLEntryData.h"
#include "Headers/Configuration.h"
#include "Headers/PlatformContext.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Scripting/Headers/Script.h"
#include "Physics/Headers/PXDevice.h"
#include "Utility/Headers/XMLParser.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Networking/Headers/Server.h"
#include "Core/Networking/Headers/LocalClient.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Platform/Compute/Headers/OpenCLInterface.h"

namespace Divide {

LoopTimingData::LoopTimingData() : _updateLoops(0),
                                   _previousTime(0ULL),
                                   _currentTime(0ULL),
                                   _currentTimeFrozen(0ULL),
                                   _currentTimeDelta(0ULL),
                                   _nextGameTick(0ULL),
                                   _keepAlive(true),
                                   _freezeLoopTime(false)
{
}

Kernel::Kernel(I32 argc, char** argv, Application& parentApp)
    : _argc(argc),
      _argv(argv),
      _APP(parentApp),
      _platformContext(nullptr),
      _resCache(nullptr),
      _taskPool(Config::MAX_POOLED_TASKS),
      _appLoopTimer(Time::ADD_TIMER("Main Loop Timer")),
      _frameTimer(Time::ADD_TIMER("Total Frame Timer")),
      _appIdleTimer(Time::ADD_TIMER("Loop Idle Timer")),
      _appScenePass(Time::ADD_TIMER("Loop Scene Pass Timer")),
      _physicsUpdateTimer(Time::ADD_TIMER("Physics Update Timer")),
      _sceneUpdateTimer(Time::ADD_TIMER("Scene Update Timer")),
      _sceneUpdateLoopTimer(Time::ADD_TIMER("Scene Update Loop timer")),
      _physicsProcessTimer(Time::ADD_TIMER("Physics Process Timer")),
      _cameraMgrTimer(Time::ADD_TIMER("Camera Manager Update Timer")),
      _flushToScreenTimer(Time::ADD_TIMER("Flush To Screen Timer")),
      _preRenderTimer(Time::ADD_TIMER("Pre-render Timer")),
      _postRenderTimer(Time::ADD_TIMER("Post-render Timer"))
{
    _platformContext = std::make_unique<PlatformContext>(
        std::make_unique<GFXDevice>(*this),              // Video
        std::make_unique<SFXDevice>(*this),              // Audio
        std::make_unique<PXDevice>(*this),               // Physics
        std::make_unique<GUI>(*this),                    // Graphical User Interface
        std::make_unique<Input::InputInterface>(*this),  // Input
        std::make_unique<XMLEntryData>(),                // Initial XML data
        std::make_unique<Configuration>(),               // XML based configuration
        std::make_unique<LocalClient>(*this));           // Network client

    _renderPassManager = std::make_unique<RenderPassManager>(*this, _platformContext->gfx());

    _sceneManager = std::make_unique<SceneManager>(*this); // Scene Manager

    _appLoopTimer.addChildTimer(_appIdleTimer);
    _appLoopTimer.addChildTimer(_frameTimer);
    _frameTimer.addChildTimer(_appScenePass);
    _appScenePass.addChildTimer(_cameraMgrTimer);
    _appScenePass.addChildTimer(_physicsUpdateTimer);
    _appScenePass.addChildTimer(_physicsProcessTimer);
    _appScenePass.addChildTimer(_sceneUpdateTimer);
    _appScenePass.addChildTimer(_flushToScreenTimer);
    _flushToScreenTimer.addChildTimer(_preRenderTimer);
    _flushToScreenTimer.addChildTimer(_postRenderTimer);
    _sceneUpdateTimer.addChildTimer(_sceneUpdateLoopTimer);

    _resCache = std::make_unique<ResourceCache>(*_platformContext);

    FrameListenerManager::createInstance();
    OpenCLInterface::createInstance();
}

Kernel::~Kernel()
{
}

void Kernel::idle() {
    _taskPool.flushCallbackQueue();

    _platformContext->idle();
    _sceneManager->idle();
    Locale::idle();
    Script::idle();

    FrameListenerManager::instance().idle();

    bool freezeLoopTime = ParamHandler::instance().getParam(_ID("freezeLoopTime"), false);
    if (freezeLoopTime != _timingData._freezeLoopTime) {
        _timingData._freezeLoopTime = freezeLoopTime;
        _timingData._currentTimeFrozen = _timingData._currentTime;
        _APP.mainLoopPaused(_timingData._freezeLoopTime);
    }
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
        _platformContext->sfx().beginFrame();
        _platformContext->gfx().beginFrame();
        {
            Time::ScopedTimer timer3(_frameTimer);
            // Launch the FRAME_STARTED event
            frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_EVENT_STARTED, evt);
            _timingData._keepAlive = frameMgr.frameEvent(evt);

            U64 deltaTime = _timingData._freezeLoopTime ? 0UL
                                                        : Config::USE_FIXED_TIMESTEP
                                                                    ? Config::SKIP_TICKS
                                                                    : _timingData._currentTimeDelta;

            // Process the current frame
            _timingData._keepAlive = mainLoopScene(evt, deltaTime) && _timingData._keepAlive;

            // Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended
            // event)
            frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_EVENT_PROCESS, evt);

            _timingData._keepAlive = frameMgr.frameEvent(evt) && _timingData._keepAlive;
        }
        _platformContext->gfx().endFrame(_APP.mainLoopActive());
        _platformContext->sfx().endFrame();

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

    U32 frameCount = _platformContext->gfx().getFrameCount();

    if (Config::Profile::BENCHMARK_PERFORMANCE || Config::Profile::ENABLE_FUNCTION_PROFILING)
    {
        // Should be approximatelly 2 times a seconds
        bool print = false;
        if (Config::Build::IS_DEBUG_BUILD) {
            print = frameCount % (Config::TARGET_FRAME_RATE / 4) == 0;
        } else {
            print = frameCount % (Config::TARGET_FRAME_RATE / 2) == 0;
        }

        if (print) {
            stringImpl profileData(Util::StringFormat("Scene Update Loops: %d", _timingData._updateLoops));

            if (Config::Profile::BENCHMARK_PERFORMANCE) {
                profileData.append("\n");
                profileData.append(Time::ApplicationTimer::instance().benchmarkReport());
                profileData.append("\n");
                profileData.append(Util::StringFormat("GPU: [ %5.5f ] [DrawCalls: %d]",
                                                      Time::MicrosecondsToSeconds<F32>(_platformContext->gfx().getFrameDurationGPU()),
                                                      _platformContext->gfx().getDrawCallCount()));
            }
            if (Config::Profile::ENABLE_FUNCTION_PROFILING) {
                profileData.append("\n");
                profileData.append(Time::ProfileTimer::printAll());
            }

            Arena::Statistics stats = _platformContext->gfx().getObjectAllocStats();
            F32 gpuAllocatedKB = stats.bytes_allocated_ / 1024.0f;

            profileData.append("\n");
            profileData.append(Util::StringFormat("GPU Objects: %5.2f Kb (%5.2f Mb),\n"
                                                  "             %d allocs,\n"
                                                  "             %d blocks,\n"
                                                  "             %d destructors",
                                                  gpuAllocatedKB,
                                                  gpuAllocatedKB / 1024,
                                                  stats.num_of_allocs_,
                                                  stats.num_of_blocks_,
                                                  stats.num_of_dtros_));
            // Should equate to approximately once every 10 seconds
            if (frameCount % (Config::TARGET_FRAME_RATE * Time::Seconds(10)) == 0) {
                Console::printfn(profileData.c_str());
            }

            _platformContext->gui().modifyText(_ID("ProfileData"), profileData);
        }

        Util::RecordFloatEvent("kernel.mainLoopApp",
                               to_F32(_appLoopTimer.get()),
                               _timingData._currentTime);

        if (frameCount % (Config::TARGET_FRAME_RATE * 10) == 0) {
            Util::FlushFloatEvents();
        }
    }
}

bool Kernel::mainLoopScene(FrameEvent& evt, const U64 deltaTime) {
    Time::ScopedTimer timer(_appScenePass);

    // Physics system uses (or should use) its own timestep code
    U64 physicsTime 
        =  _timingData._freezeLoopTime ? 0ULL
                                       : _timingData._currentTimeDelta;
    {
        Time::ScopedTimer timer2(_cameraMgrTimer);
        // Update cameras
        Camera::update(deltaTime);
    }

    if (_APP.windowManager().getActiveWindow().minimized()) {
        idle();
        return true;
    }

    {
        Time::ScopedTimer timer2(_physicsProcessTimer);
        // Process physics
        _platformContext->pfx().process(physicsTime);
    }

    {
        Time::ScopedTimer timer2(_sceneUpdateTimer);

        const SceneManager::PlayerList& activePlayers = _sceneManager->getPlayers();
        U8 playerCount = to_U8(activePlayers.size());

        _timingData._updateLoops = 0;
        while (_timingData._currentTime > _timingData._nextGameTick &&
            _timingData._updateLoops < Config::MAX_FRAMESKIP) {

            if (_timingData._updateLoops == 0) {
                _sceneUpdateLoopTimer.start();
            }

            _sceneManager->processGUI(deltaTime);

            // Flush any pending threaded callbacks
            _taskPool.flushCallbackQueue();
            // Update scene based on input
            for (U8 i = 0; i < playerCount; ++i) {
                _sceneManager->processInput(i, deltaTime);
            }
            // process all scene events
            _sceneManager->processTasks(deltaTime);
            // Update the scene state based on current time (e.g. animation matrices)
            _sceneManager->updateSceneState(deltaTime);
            // Update visual effect timers as well
            PostFX::instance().update(deltaTime);
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

    U32 frameCount = _platformContext->gfx().getFrameCount();

    if (frameCount % (Config::TARGET_FRAME_RATE / Config::Networking::NETWORK_SEND_FREQUENCY_HZ) == 0) {
        U32 retryCount = 0;
        while (!Attorney::SceneManagerKernel::networkUpdate(*_sceneManager, frameCount)) {
            if (retryCount > Config::Networking::NETWORK_SEND_RETRY_COUNT) {
                break;
            }
        }
    }


    D64 interpolationFactor = 1.0;
    if (Config::USE_FIXED_TIMESTEP && !_timingData._freezeLoopTime) {
        interpolationFactor = static_cast<D64>(_timingData._currentTime + deltaTime - _timingData._nextGameTick) / deltaTime;
        assert(interpolationFactor <= 1.0 && interpolationFactor > 0.0);
    }

    GFXDevice::setFrameInterpolationFactor(interpolationFactor);
    
    // Get input events
    if (_APP.windowManager().getActiveWindow().hasFocus()) {
        _platformContext->input().update(_timingData._currentTimeDelta);
    } else {
        _sceneManager->onLostFocus();
    }
    {
        Time::ScopedTimer timer3(_physicsUpdateTimer);
        // Update physics
        _platformContext->pfx().update(physicsTime);
    }

    // Update the graphical user interface
    const OIS::MouseState& mouseState = _platformContext->input().getKeyboardMousePair(0).second->getMouseState();
    _platformContext->gui().update(_timingData._currentTimeDelta);
    _platformContext->gui().setCursorPosition(mouseState.X.abs, mouseState.Y.abs);
    return presentToScreen(evt, deltaTime);
}

void computeViewports(const vec4<I32>& mainViewport, vectorImpl<vec4<I32>>& targetViewports, U8 count) {
    
    assert(count > 0);
    I32 xOffset = mainViewport.x;
    I32 yOffset = mainViewport.y;
    I32 width = mainViewport.z;
    I32 height = mainViewport.w;

    targetViewports.resize(0);
    if (count == 1) { //Single Player
        targetViewports.push_back(mainViewport);
        return;
    } else if (count == 2) { //Split Screen
        I32 halfHeight = height / 2;
        targetViewports.emplace_back(xOffset, halfHeight + yOffset, width, halfHeight);
        targetViewports.emplace_back(xOffset, 0 + yOffset,          width, halfHeight);
        return;
    }

    // Basic idea (X - viewport):
    // Odd # of players | Even # of players
    // X X              |      X X
    //  X               |      X X
    //                  |
    // X X X            |     X X X 
    //  X X             |     X X X
    // etc
    // Always try to match last row with previous one
    // If they match, move the first viewport from the last row
    // to the previous row and add a new entry to the end of the
    // current row

    typedef vectorImpl<vec4<I32>> ViewportRow;
    typedef vectorImpl<ViewportRow> ViewportRows;
    ViewportRows rows;

    // Allocates storage for a N x N matrix of viewports that will hold numViewports
    // Returns N;
    auto resizeViewportContainer = [&rows](U32 numViewports) {
        //Try to fit all viewports into an appropriately sized matrix.
        //If the number of resulting rows is too large, drop empty rows.
        //If the last row has an odd number of elements, center them later.
        U8 matrixSize = to_U8(minSquareMatrixSize(numViewports));
        rows.resize(matrixSize);
        std::for_each(std::begin(rows), std::end(rows), [matrixSize](ViewportRow& row) { row.resize(matrixSize); });

        return matrixSize;
    };

    // Remove extra rows and columns, if any
    U8 columnCount = resizeViewportContainer(count);
    U8 extraColumns = (columnCount * columnCount) - count;
    U8 extraRows = extraColumns / columnCount;
    for (U8 i = 0; i < extraRows; ++i) {
        rows.pop_back();
    }
    U8 columnsToRemove = extraColumns - (extraRows * columnCount);
    for (U8 i = 0; i < columnsToRemove; ++i) {
        rows.back().pop_back();
    }

    U8 rowCount = to_U8(rows.size());

    // Calculate and set viewport dimensions
    // The number of columns is valid for the width;
    I32 playerWidth = width / columnCount;
    // The number of rows is valid for the height;
    I32 playerHeight = height / to_I32(rowCount);

    for (U8 i = 0; i < rowCount; ++i) {
        ViewportRow& row = rows[i];
        I32 playerYOffset = playerHeight * (rowCount - i - 1);
        for (U8 j = 0; j < to_U8(row.size()); ++j) {
            I32 playerXOffset = playerWidth * j;
            row[j].set(playerXOffset, playerYOffset, playerWidth, playerHeight);
        }
    }

    //Slide the last row to center it
    if (extraColumns > 0) {
        ViewportRow& lastRow = rows.back();
        I32 screenMidPoint = width / 2;
        I32 rowMidPoint = to_I32((lastRow.size() * playerWidth) / 2);
        I32 slideFactor = screenMidPoint - rowMidPoint;
        for (vec4<I32>& viewport : lastRow) {
            viewport.x += slideFactor;
        }
    }

    // Update the system viewports
    for (const ViewportRow& row : rows) {
        for (const vec4<I32>& viewport : row) {
            targetViewports.push_back(viewport);
        }
    }
}

Time::ProfileTimer& getTimer(Time::ProfileTimer& parentTimer, vectorImpl<Time::ProfileTimer*>& timers, U8 index, const char* name) {
    while (timers.size() < index + 1) {
        timers.push_back(&Time::ADD_TIMER(Util::StringFormat("%s %d", name, timers.size()).c_str()));
        parentTimer.addChildTimer(*timers.back());
    }

    return *timers[index];
}

bool Kernel::presentToScreen(FrameEvent& evt, const U64 deltaTime) {
    static vectorImpl<vec4<I32>> targetViewports;

    Time::ScopedTimer time(_flushToScreenTimer);

    FrameListenerManager& frameMgr = FrameListenerManager::instance();

    {
        Time::ScopedTimer time1(_preRenderTimer);
        frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_PRERENDER_START, evt);

        if (!frameMgr.frameEvent(evt)) {
            return false;
        }

        // perform time-sensitive shader tasks
        if (!ShaderProgram::updateAll(deltaTime)) {
            return false;
        }

        frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_PRERENDER_END, evt);
        if (!frameMgr.frameEvent(evt)) {
            return false;
        }
    }

    const SceneManager::PlayerList& activePlayers = _sceneManager->getPlayers();
    const vec4<I32>& mainViewport = _platformContext->gfx().getCurrentViewport();

    U8 playerCount = to_U8(activePlayers.size());
    computeViewports(mainViewport, targetViewports, playerCount);

    for (U8 i = 0; i < playerCount; ++i) {
        Attorney::SceneManagerKernel::currentPlayerPass(*_sceneManager, i);
        {
            Time::ScopedTimer time2(getTimer(_flushToScreenTimer, _renderTimer, i, "Render Timer"));
            _renderPassManager->render(_sceneManager->getActiveScene().renderState());
        }
        {
            Time::ScopedTimer time3(getTimer(_flushToScreenTimer, _postFxRenderTimer, i, "PostFX Timer"));
            PostFX::instance().apply(_platformContext->gfx());
        }
        {
            Time::ScopedTimer time4(getTimer(_flushToScreenTimer, _blitToDisplayTimer, i, "Blit to screen Timer"));
            Attorney::GFXDeviceKernel::flushDisplay(_platformContext->gfx(), targetViewports[i]);
        }
    }
    Camera::activeCamera(nullptr);

    for (U8 i = playerCount; i < to_U8(_renderTimer.size()); ++i) {
        Time::ProfileTimer::removeTimer(*_renderTimer[i]);
        Time::ProfileTimer::removeTimer(*_postFxRenderTimer[i]);
        Time::ProfileTimer::removeTimer(*_blitToDisplayTimer[i]);
        _renderTimer.erase(std::begin(_renderTimer) + i);
        _postFxRenderTimer.erase(std::begin(_postFxRenderTimer) + i);
        _blitToDisplayTimer.erase(std::begin(_blitToDisplayTimer) + i);
    }

    {
        Time::ScopedTimer time5(_postRenderTimer);
        frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_POSTRENDER_START, evt);

        if (!frameMgr.frameEvent(evt)) {
            return false;
        }

        frameMgr.createEvent(_timingData._currentTime, FrameEventType::FRAME_POSTRENDER_END, evt);

        if (!frameMgr.frameEvent(evt)) {
            return false;
        }
    }

    return true;
}

// The first loops compiles all the visible data, so do not render the first couple of frames
void Kernel::warmup() {
    Console::printfn(Locale::get(_ID("START_RENDER_LOOP")));
    _timingData._nextGameTick = Time::ElapsedMicroseconds(true);
    _timingData._keepAlive = true;

    if (false) {
        static const U8 warmupLoopCount = 3;
        U8 loopCount = 0;

        RenderDetailLevel shadowLevel = _platformContext->gfx().shadowDetailLevel();
        ParamHandler::instance().setParam(_ID("freezeLoopTime"), true);
        _platformContext->gfx().shadowDetailLevel(RenderDetailLevel::OFF);

        onLoop();
        loopCount++;

        if (shadowLevel != RenderDetailLevel::OFF) {
            _platformContext->gfx().shadowDetailLevel(shadowLevel);
            onLoop();
            loopCount++;
        }

        for (U8 i = 0; i < std::max(warmupLoopCount - loopCount, 0); ++i) {
            onLoop();
        }

    }

    ParamHandler::instance().setParam(_ID("freezeLoopTime"), false);
    Attorney::SceneManagerKernel::initPostLoadState(*_sceneManager);

    _timingData._currentTime = _timingData._nextGameTick = Time::ElapsedMicroseconds(true);
}

ErrorCode Kernel::initialize(const stringImpl& entryPoint) {
    const SysInfo& systemInfo = const_sysInfo();
    if (Config::REQUIRED_RAM_SIZE > systemInfo._availableRam) {
        return ErrorCode::NOT_ENOUGH_RAM;
    }

    // We have an A.I. thread, a networking thread, a PhysX thread, the main
    // update/rendering thread so how many threads do we allocate for tasks?
    // That's up to the programmer to decide for each app.
    if (!_taskPool.init(HARDWARE_THREAD_COUNT(), TaskPool::TaskPoolType::PRIORITY_QUEUE, "DIVIDE_WORKER_THREAD_")) {
        return ErrorCode::CPU_NOT_SUPPORTED;
    }

    _platformContext->pfx().setAPI(PXDevice::PhysicsAPI::PhysX);
    _platformContext->sfx().setAPI(SFXDevice::AudioAPI::SDL);
    // Using OpenGL for rendering as default
    if (Config::USE_OPENGL_RENDERING) {
        _platformContext->gfx().setAPI(Config::USE_OPENGL_ES ? RenderAPI::OpenGLES
                                                             : RenderAPI::OpenGL);
    }
    // Load info from XML files
    XMLEntryData& entryData = _platformContext->entryData();
    Configuration& config = _platformContext->config();
    XML::loadFromXML(entryData, entryPoint.c_str());
    XML::loadFromXML(config, (entryData.scriptLocation + "/config.xml").c_str());

    Server::instance().init(((Divide::U16)443), "127.0.0.1", true);

    if (!_platformContext->client().connect(entryData.serverAddress, 443)) {
        _platformContext->client().connect("127.0.0.1", 443);
    }

    Paths::updatePaths(*_platformContext);

    Locale::changeLanguage(config.language);

    _platformContext->gfx().shadowDetailLevel(config.rendering.shadowDetailLevel);
    _platformContext->gfx().renderDetailLevel(config.rendering.renderDetailLevel);

    // Create mem log file
    const stringImpl& mem = config.debug.memFile;
    _APP.setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get(_ID("START_RENDER_INTERFACE")));

    // Fulscreen is automatically calculated
    ResolutionByType initRes;
    initRes[to_base(WindowType::WINDOW)].set(config.runtime.resolution.w, config.runtime.resolution.h);
    initRes[to_base(WindowType::SPLASH)].set(config.runtime.splashScreen.w, config.runtime.splashScreen.h);

    bool startFullScreen = !config.runtime.windowedMode;
    WindowManager& winManager = _APP.windowManager();

    ErrorCode initError = winManager.init(*_platformContext, _platformContext->gfx().getAPI(), initRes, startFullScreen, config.runtime.targetDisplay);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Match initial rendering resolution to window/screen size
    const DisplayWindow& mainWindow  = winManager.getActiveWindow();
    vec2<U16> renderResolution(startFullScreen ? mainWindow.getDimensions(WindowType::FULLSCREEN)
                                               : mainWindow.getDimensions(WindowType::WINDOW));
    initError = _platformContext->gfx().initRenderingAPI(_argc, _argv, renderResolution);

    Camera::addUpdateListener([this](const Camera& cam) { Attorney::GFXDeviceKernel::onCameraUpdate(_platformContext->gfx(), cam); });
    Camera::addChangeListener([this](const Camera& cam) { Attorney::GFXDeviceKernel::onCameraChange(_platformContext->gfx(), cam); });
    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }
    initError = OpenCLInterface::instance().init();
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Add our needed app-wide render passes. RenderPassManager is responsible for deleting these!
    _renderPassManager->init();
    _renderPassManager->addRenderPass("shadowPass",     0, RenderStage::SHADOW);
    _renderPassManager->addRenderPass("reflectionPass", 1, RenderStage::REFLECTION);
    _renderPassManager->addRenderPass("refractionPass", 2, RenderStage::REFRACTION);
    _renderPassManager->addRenderPass("displayStage",   3, RenderStage::DISPLAY);

    Console::printfn(Locale::get(_ID("SCENE_ADD_DEFAULT_CAMERA")));

    Script::onStartup();
    SceneManager::onStartup();
    // We start of with a forward plus renderer
    _platformContext->gfx().setRenderer(RendererType::RENDERER_TILED_FORWARD_SHADING);

    DisplayWindow& window = winManager.getActiveWindow();
    window.type(WindowType::SPLASH);
    winManager.handleWindowEvent(WindowEvent::APP_LOOP, -1, -1, -1);
    // Load and render the splash screen
    _platformContext->gfx().beginFrame();
    GUISplash(*_resCache, "divideLogo.jpg", initRes[to_base(WindowType::SPLASH)]).render(_platformContext->gfx());
    _platformContext->gfx().endFrame(true);

    Console::printfn(Locale::get(_ID("START_SOUND_INTERFACE")));
    initError = _platformContext->sfx().initAudioAPI(*_platformContext);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    Console::printfn(Locale::get(_ID("START_PHYSICS_INTERFACE")));
    initError = _platformContext->pfx().initPhysicsAPI(Config::TARGET_FRAME_RATE, config.runtime.simSpeed);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Bind the kernel with the input interface
    initError = _platformContext->input().init(*this, renderResolution);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    // Initialize GUI with our current resolution
    _platformContext->gui().init(*_platformContext, *_resCache, renderResolution);
    _platformContext->gui().addText(_ID("ProfileData"),                            // Unique ID
                                    vec2<I32>(renderResolution.width * 0.75, 100), // Position
                                    Font::DROID_SERIF_BOLD,                        // Font
                                    vec4<U8>(255,  50, 0, 255),                    // Colour
                                    "PROFILE DATA",                                // Text
                                    12);                                           // Font size

    Console::bindConsoleOutput([this](const char* output, bool error) {
                                   _platformContext->gui().getConsole()->printText(output, error);
                               });
    
    ShadowMap::initShadowMaps(_platformContext->gfx());
    _sceneManager->init(*_platformContext, *_resCache);
    if (!_sceneManager->switchScene(entryData.startupScene, true, false)) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")), entryData.startupScene.c_str());
        return ErrorCode::MISSING_SCENE_DATA;
    }

    if (!_sceneManager->checkLoadFlag()) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_NOT_CALLED")),
                         entryData.startupScene.c_str());
        return ErrorCode::MISSING_SCENE_LOAD_CALL;
    }

    Console::printfn(Locale::get(_ID("INITIAL_DATA_LOADED")));

    return initError;
}

void Kernel::shutdown() {
    Console::printfn(Locale::get(_ID("STOP_KERNEL")));
    WaitForAllTasks(_taskPool, true, true, true);
    SceneManager::onShutdown();
    Script::onShutdown();
    _sceneManager.reset();
    // release the scene
    Console::bindConsoleOutput(std::function<void(const char*, bool)>());
    _platformContext->gui().destroy();  /// Deactivate GUI 
    Camera::destroyPool();
    ShadowMap::clearShadowMaps(_platformContext->gfx());
    Console::printfn(Locale::get(_ID("STOP_ENGINE_OK")));
    Console::printfn(Locale::get(_ID("STOP_PHYSICS_INTERFACE")));
    _platformContext->pfx().closePhysicsAPI();
    Console::printfn(Locale::get(_ID("STOP_HARDWARE")));
    OpenCLInterface::instance().deinit();
    _renderPassManager.reset();
    _platformContext->sfx().closeAudioAPI();
    _platformContext->gfx().closeRenderingAPI();
    _platformContext->input().terminate();
    _platformContext->terminate();
    _resCache->clear();
    FrameListenerManager::destroyInstance();
    Camera::destroyPool();
}

void Kernel::onChangeWindowSize(U16 w, U16 h) {
    Attorney::GFXDeviceKernel::onChangeWindowSize(_platformContext->gfx(), w, h);

    if (_platformContext->input().isInit()) {
        _platformContext->input().onChangeWindowSize(w, h);
    }

    _platformContext->gui().onChangeResolution(w, h);
}

void Kernel::onChangeRenderResolution(U16 w, U16 h) const {
    Attorney::GFXDeviceKernel::onChangeRenderResolution(_platformContext->gfx(), w, h);
    _platformContext->gui().onChangeResolution(w, h);

    _sceneManager->onChangeResolution(w, h);
}

///--------------------------Input Management-------------------------------------///
bool Kernel::setCursorPosition(I32 x, I32 y) const {
    _platformContext->gui().setCursorPosition(x, y);
    return true;
}

bool Kernel::onKeyDown(const Input::KeyEvent& key) {
    if (_platformContext->gui().onKeyDown(key)) {
        return _sceneManager->onKeyDown(key);
    }
    return true;  //< InputInterface needs to know when this is completed
}

bool Kernel::onKeyUp(const Input::KeyEvent& key) {
    if (_platformContext->gui().onKeyUp(key)) {
        return _sceneManager->onKeyUp(key);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseMoved(const Input::MouseEvent& arg) {
    if (_platformContext->gui().mouseMoved(arg)) {
        return _sceneManager->mouseMoved(arg);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonPressed(const Input::MouseEvent& arg,
                                Input::MouseButton button) {
    if (_platformContext->gui().mouseButtonPressed(arg, button)) {
        return _sceneManager->mouseButtonPressed(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonReleased(const Input::MouseEvent& arg,
                                 Input::MouseButton button) {
    if (_platformContext->gui().mouseButtonReleased(arg, button)) {
        return _sceneManager->mouseButtonReleased(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (_platformContext->gui().joystickAxisMoved(arg, axis)) {
        return _sceneManager->joystickAxisMoved(arg, axis);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (_platformContext->gui().joystickPovMoved(arg, pov)) {
        return _sceneManager->joystickPovMoved(arg, pov);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonPressed(const Input::JoystickEvent& arg,
                                   Input::JoystickButton button) {
    if (_platformContext->gui().joystickButtonPressed(arg, button)) {
        return _sceneManager->joystickButtonPressed(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonReleased(const Input::JoystickEvent& arg,
                                    Input::JoystickButton button) {
    if (_platformContext->gui().joystickButtonReleased(arg, button)) {
        return _sceneManager->joystickButtonReleased(arg, button);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    if (_platformContext->gui().joystickSliderMoved(arg, index)) {
        return _sceneManager->joystickSliderMoved(arg, index);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index) {
    if (_platformContext->gui().joystickVector3DMoved(arg, index)) {
        return _sceneManager->joystickVector3DMoved(arg, index);
    }
    // InputInterface needs to know when this is completed
    return false;
}

void Attorney::KernelApplication::syncThreadToGPU(Kernel& kernel, const std::thread::id& threadID, bool beginSync) {
    Attorney::GFXDeviceKernel::syncThreadToGPU(kernel._platformContext->gfx(), threadID, beginSync);
}

};
