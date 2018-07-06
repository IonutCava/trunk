#include "stdafx.h"

#include "config.h"

#include "Headers/Kernel.h"
#include "Headers/XMLEntryData.h"
#include "Headers/Configuration.h"
#include "Headers/PlatformContext.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUISplash.h"
#include "GUI/Headers/GUIConsole.h"
#include "Editor/Headers/Editor.h"
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
#include "Core/Debugging/Headers/DebugInterface.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/File/Headers/FileWatcherManager.h"
#include "Platform/Compute/Headers/OpenCLInterface.h"

#include <AntTweakBar/include/AntTweakBar.h>

namespace Divide {

LoopTimingData::LoopTimingData() : _updateLoops(0),
                                   _previousTimeUS(0ULL),
                                   _currentTimeUS(0ULL),
                                   _currentTimeFrozenUS(0ULL),
                                   _currentTimeDeltaUS(0ULL),
                                   _nextGameTickUS(0ULL),
                                   _keepAlive(true),
                                   _freezeLoopTime(false)
{
}

Kernel::Kernel(I32 argc, char** argv, Application& parentApp)
    : _argc(argc),
      _argv(argv),
      _resCache(nullptr),
      _platformContext(std::make_unique<PlatformContext>(parentApp, *this)),
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
    _platformContext->init();

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

void Kernel::startSplashScreen() {
    if (_splashScreenUpdating) {
        return;
    }

    _splashScreenUpdating = true;
    Configuration& config = _platformContext->config();
    GUISplash splash(*_resCache, "divideLogo.jpg", config.runtime.splashScreen);

    // Load and render the splash screen
    _splashTask = CreateTask(*_platformContext,
        [this, &splash](const Task& /*task*/) {
        U64 previousTimeUS = 0;
        U64 currentTimeUS = Time::ElapsedMicroseconds(true);
        while (_splashScreenUpdating) {
            U64 deltaTimeUS = currentTimeUS - previousTimeUS;
            previousTimeUS = currentTimeUS;
            _platformContext->beginFrame();
            splash.render(_platformContext->gfx(), deltaTimeUS);
            _platformContext->endFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(15));

            break;
        }
    });
    _splashTask._task->startTask(Task::TaskPriority::REALTIME/*HIGH*/);
}

void Kernel::stopSplashScreen() {
    _splashScreenUpdating = false;
    _splashTask.wait();
}

void Kernel::idle() {
    _taskPool.flushCallbackQueue();

    _platformContext->idle();
    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcherManager::idle();
    }
    _sceneManager->idle();
    Locale::idle();
    Script::idle();
    FrameListenerManager::instance().idle();

    bool freezeLoopTime = ParamHandler::instance().getParam(_ID("freezeLoopTime"), false);

    if (Config::Build::ENABLE_EDITOR) {
        freezeLoopTime |= _platformContext->editor().shouldPauseSimulation();
    }

    if (_timingData.freezeTime(freezeLoopTime)) {
        _platformContext->app().mainLoopPaused(freezeLoopTime);
    }
}

void Kernel::onLoop() {
    if (!_timingData._keepAlive) {
        // exiting the rendering loop will return us to the last control point
        _platformContext->app().mainLoopActive(false);
        return;
    }

    // Update internal timer
    Time::ApplicationTimer::instance().update();
    {
        Time::ScopedTimer timer(_appLoopTimer);
   
        // Update time at every render loop
        _timingData.update(Time::ElapsedMicroseconds());
        {
            Time::ScopedTimer timer2(_appIdleTimer);
            idle();
        }
        FrameEvent evt;
        FrameListenerManager& frameMgr = FrameListenerManager::instance();

        // Restore GPU to default state: clear buffers and set default render state
        _platformContext->beginFrame();
        {
            Time::ScopedTimer timer3(_frameTimer);
            // Launch the FRAME_STARTED event
            _timingData._keepAlive = frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(), FrameEventType::FRAME_EVENT_STARTED, evt);

            U64 deltaTimeUS = 0ULL;
            if (!_timingData.freezeTime()) {
                deltaTimeUS = Config::USE_FIXED_TIMESTEP ? Time::SecondsToMicroseconds(1) / Config::TICKS_PER_SECOND
                                                         : _timingData.currentTimeDeltaUS();
            }

            // Process the current frame
            _timingData._keepAlive = mainLoopScene(evt, deltaTimeUS) && _timingData._keepAlive;

            // Launch the FRAME_PROCESS event (a.k.a. the frame processing has ended event)
            _timingData._keepAlive = _timingData._keepAlive && frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_EVENT_PROCESS, evt);
        }
        _platformContext->endFrame();

        // Launch the FRAME_ENDED event (buffers have been swapped)

        _timingData._keepAlive = _timingData._keepAlive && frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_EVENT_ENDED, evt);

        _timingData._keepAlive = !_platformContext->app().ShutdownRequested() && _timingData._keepAlive;
    
        ErrorCode err = _platformContext->app().errorCode();

        if (err != ErrorCode::NO_ERR) {
            Console::errorfn("Error detected: [ %s ]", getErrorCodeName(err));
            _timingData._keepAlive = false;
        }
    }

    U32 frameCount = _platformContext->gfx().getFrameCount();

    if (Config::Profile::BENCHMARK_PERFORMANCE || Config::Profile::ENABLE_FUNCTION_PROFILING)
    {
        // Should be approximately 2 times a seconds
        bool print = false;
        if (Config::Build::IS_DEBUG_BUILD) {
            print = frameCount % (Config::TARGET_FRAME_RATE / 4) == 0;
        } else {
            print = frameCount % (Config::TARGET_FRAME_RATE / 2) == 0;
        }

        if (print) {
            stringImpl profileData(Util::StringFormat("Scene Update Loops: %d", _timingData.updateLoops()));

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

        Util::RecordFloatEvent("kernel.mainLoopApp", to_F32(_appLoopTimer.get()), _timingData.currentTimeUS());

        if (frameCount % (Config::TARGET_FRAME_RATE * 10) == 0) {
            Util::FlushFloatEvents();
        }
    }

    // Cap FPS
    I16 frameLimit = _platformContext->config().runtime.frameRateLimit;
    if (frameLimit > 0) {
        F32 deltaMilliseconds = Time::MicrosecondsToMilliseconds<F32>(_timingData.currentTimeDeltaUS());
        F32 targetFrametime = 1000.0f / frameLimit;

        if (deltaMilliseconds < targetFrametime) {
            //Sleep the remaining frame time 
            std::this_thread::sleep_for(std::chrono::milliseconds(to_I32(std::floorf(targetFrametime - deltaMilliseconds))));
        }
    }
}

bool Kernel::mainLoopScene(FrameEvent& evt, const U64 deltaTimeUS) {
    Time::ScopedTimer timer(_appScenePass);

    // Physics system uses (or should use) its own timestep code
    U64 realTime = _timingData.currentTimeDeltaUS();
    {
        Time::ScopedTimer timer2(_cameraMgrTimer);
        // Update cameras
        Camera::update(deltaTimeUS);
    }

    if (_platformContext->app().windowManager().getActiveWindow().minimized()) {
        idle();
        return true;
    }

    {
        Time::ScopedTimer timer2(_physicsProcessTimer);
        // Process physics
        _platformContext->pfx().process(realTime);
    }

    {
        Time::ScopedTimer timer2(_sceneUpdateTimer);

        const SceneManager::PlayerList& activePlayers = _sceneManager->getPlayers();
        U8 playerCount = to_U8(activePlayers.size());

        bool firstRun = true;
        while (_timingData.runUpdateLoop()) {

            if (firstRun) {
                _sceneUpdateLoopTimer.start();
            }

            _sceneManager->processGUI(deltaTimeUS);

            // Flush any pending threaded callbacks
            _taskPool.flushCallbackQueue();
            // Update scene based on input
            for (U8 i = 0; i < playerCount; ++i) {
                _sceneManager->processInput(i, deltaTimeUS);
            }
            // process all scene events
            _sceneManager->processTasks(deltaTimeUS);
            // Update the scene state based on current time (e.g. animation matrices)
            _sceneManager->updateSceneState(deltaTimeUS);
            // Update visual effect timers as well
            PostFX::instance().update(deltaTimeUS);

            if (firstRun) {
                _sceneUpdateLoopTimer.stop();
                firstRun = false;
            }

            _timingData.endUpdateLoop(deltaTimeUS);
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
    if (Config::USE_FIXED_TIMESTEP && !_timingData.freezeTime()) {
        interpolationFactor = static_cast<D64>(_timingData.currentTimeUS() + deltaTimeUS - _timingData.nextGameTickUS()) / deltaTimeUS;
        assert(interpolationFactor <= 1.0 && interpolationFactor > 0.0);
    }

    GFXDevice::setFrameInterpolationFactor(interpolationFactor);
    
    // Update windows and get input events
    WindowManager& winManager = _platformContext->app().windowManager();
    winManager.update(realTime);
    if (!winManager.anyWindowFocus()) {
        _sceneManager->onLostFocus();
    }

    {
        Time::ScopedTimer timer3(_physicsUpdateTimer);
        // Update physics
        _platformContext->pfx().update(realTime);
    }

    // Update the graphical user interface
    _platformContext->gui().update(deltaTimeUS);

    if (Config::Build::ENABLE_EDITOR) {
        _platformContext->editor().update(realTime);
    }

    return presentToScreen(evt, deltaTimeUS);
}

void computeViewports(const Rect<I32>& mainViewport, vectorImpl<Rect<I32>>& targetViewports, U8 count) {
    
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

    typedef vectorImpl<Rect<I32>> ViewportRow;
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
        for (Rect<I32>& viewport : lastRow) {
            viewport.x += slideFactor;
        }
    }

    // Update the system viewports
    for (const ViewportRow& row : rows) {
        for (const Rect<I32>& viewport : row) {
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

bool Kernel::presentToScreen(FrameEvent& evt, const U64 deltaTimeUS) {
    static vectorImpl<Rect<I32>> targetViewports;

    Time::ScopedTimer time(_flushToScreenTimer);

    FrameListenerManager& frameMgr = FrameListenerManager::instance();

    {
        Time::ScopedTimer time1(_preRenderTimer);
        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_PRERENDER_START, evt)) {
            return false;
        }

        // perform time-sensitive shader tasks
        if (!ShaderProgram::updateAll(deltaTimeUS)) {
            return false;
        }

        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_PRERENDER_END, evt)) {
            return false;
        }
    }

    const SceneManager::PlayerList& activePlayers = _sceneManager->getPlayers();
    const Rect<I32>& mainViewport = _platformContext->gfx().getCurrentViewport();

    U8 playerCount = to_U8(activePlayers.size());
    computeViewports(mainViewport, targetViewports, playerCount);

    for (U8 i = 0; i < playerCount; ++i) {
        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_SCENERENDER_START, evt)) {
            return false;
        }

        Attorney::SceneManagerKernel::currentPlayerPass(*_sceneManager, i);
        {
            Time::ScopedTimer time2(getTimer(_flushToScreenTimer, _renderTimer, i, "Render Timer"));
            _renderPassManager->render(_sceneManager->getActiveScene().renderState());
        }
        {
            Time::ScopedTimer time3(getTimer(_flushToScreenTimer, _postFxRenderTimer, i, "PostFX Timer"));
            PostFX::instance().apply();
        }

        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_SCENERENDER_END, evt)) {
            return false;
        }
        {
            Time::ScopedTimer time4(getTimer(_flushToScreenTimer, _blitToDisplayTimer, i, "Blit to screen Timer"));
            if (Config::Build::ENABLE_EDITOR && _platformContext->editor().running()) {
                Attorney::GFXDeviceKernel::blitToRenderTarget(_platformContext->gfx(), RenderTargetID(RenderTargetUsage::EDITOR), targetViewports[i]);
            } else {
                Attorney::GFXDeviceKernel::blitToScreen(_platformContext->gfx(), targetViewports[i]);
            }
        }
    }

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
        if(!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_POSTRENDER_START, evt)) {
            return false;
        }

        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_POSTRENDER_END, evt)) {
            return false;
        }
    }

    return true;
}

// The first loops compiles all the visible data, so do not render the first couple of frames
void Kernel::warmup() {
    Console::printfn(Locale::get(_ID("START_RENDER_LOOP")));

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

    _timingData.update(Time::ElapsedMicroseconds(true));
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

    ASIO::SET_LOG_FUNCTION([](const char* msg, bool is_error) {
        is_error ? Console::errorfn(msg) : Console::printfn(msg);
    });

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

    Locale::changeLanguage(config.language.c_str());
    ECS::Initialize();

    _platformContext->gfx().shadowDetailLevel(config.rendering.shadowDetailLevel);
    _platformContext->gfx().renderDetailLevel(config.rendering.renderDetailLevel);

    // Create mem log file
    const stringImpl& mem = config.debug.memFile;
    _platformContext->app().setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get(_ID("START_RENDER_INTERFACE")));

    // Fullscreen is automatically calculated
    bool startFullScreen = !config.runtime.windowedMode;
    WindowManager& winManager = _platformContext->app().windowManager();

    ErrorCode initError = winManager.init(*_platformContext, _platformContext->gfx().getAPI(), config.runtime.resolution, startFullScreen, config.runtime.targetDisplay);
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    Camera::initPool();
    // Match initial rendering resolution to window/screen size
    const DisplayWindow& mainWindow  = winManager.getActiveWindow();
    vec2<U16> renderResolution(mainWindow.getDimensions());
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
    Attorney::ShaderProgramKernel::useShaderTextCache(config.debug.useShaderTextCache);
    Attorney::ShaderProgramKernel::useShaderBinaryCache(config.debug.useShaderBinaryCache);

    DisplayWindow& window = winManager.getActiveWindow();
    window.setDimensions(config.runtime.splashScreen);
    window.changeType(WindowType::SPLASH);
    winManager.handleWindowEvent(WindowEvent::APP_LOOP, -1, -1, -1);

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

    // Initialize GUI with our current resolution
    _platformContext->gui().init(*_platformContext, *_resCache, renderResolution);
    _platformContext->gui().addText("ProfileData",                                 // Unique ID
                                    RelativePosition2D(RelativeValue(0.75f, 0.0f),
                                                       RelativeValue(0.1f, 0.0f)), // Position
                                    Font::DROID_SERIF_BOLD,                        // Font
                                    UColour(255,  50, 0, 255),                     // Colour
                                    "PROFILE DATA",                                // Text
                                    12);                                           // Font size

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
    if (Config::Build::ENABLE_EDITOR) {
        if (!_platformContext->editor().init(renderResolution)) {
            return ErrorCode::EDITOR_INIT_ERROR;
        }
        _sceneManager->addSelectionCallback([&](PlayerIndex idx, SceneGraphNode* node) {
            _platformContext->editor().selectionChangeCallback(idx, node);
        });
    }

    Console::printfn(Locale::get(_ID("INITIAL_DATA_LOADED")));

    return initError;
}

void Kernel::shutdown() {
    Console::printfn(Locale::get(_ID("STOP_KERNEL")));
    WaitForAllTasks(_taskPool, true, true, true);
    if (Config::Build::ENABLE_EDITOR) {
        _platformContext->editor().toggle(false);
    }
    SceneManager::onShutdown();
    Script::onShutdown();
    _sceneManager.reset();
    ECS::Terminate();

    ShadowMap::clearShadowMaps(_platformContext->gfx());
    OpenCLInterface::instance().deinit();
    _renderPassManager.reset();

    _taskPool.shutdown();
    Camera::destroyPool();
    _platformContext->terminate();
    _resCache->clear();
    FrameListenerManager::destroyInstance();

    Console::printfn(Locale::get(_ID("STOP_ENGINE_OK")));
}

void Kernel::onSizeChange(const SizeChangeParams& params) const {
    Attorney::GFXDeviceKernel::onSizeChange(_platformContext->gfx(), params);
    _platformContext->gui().onSizeChange(params);

    if (params.isWindowResize) {
        if (Config::USE_ANT_TWEAK_BAR) {
            TwWindowSize(to_I32(params.width), to_I32(params.height));
        }
    } else {
        _sceneManager->onSizeChange(params);
    }

    if (Config::Build::ENABLE_EDITOR) {
        _platformContext->editor().onSizeChange(params);
    }
}

///--------------------------Input Management-------------------------------------///
bool Kernel::setCursorPosition(I32 x, I32 y) {
    _platformContext->gui().setCursorPosition(x, y);
    return true;
}

bool Kernel::onKeyDown(const Input::KeyEvent& key) {
    if (Config::Build::ENABLE_EDITOR) {
        if (_platformContext->editor().onKeyDown(key)) {
            return true;
        }
    }

    if (!_platformContext->gui().onKeyDown(key)) {
        return _sceneManager->onKeyDown(key);
    }
    return true;  //< InputInterface needs to know when this is completed
}

bool Kernel::onKeyUp(const Input::KeyEvent& key) {
    if (Config::Build::ENABLE_EDITOR) {
        if (_platformContext->editor().onKeyUp(key)) {
            return true;
        }
    }

    if (!_platformContext->gui().onKeyUp(key)) {
        return _sceneManager->onKeyUp(key);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseMoved(const Input::MouseEvent& arg) {
    if (Config::Build::ENABLE_EDITOR) {
        if (_platformContext->editor().mouseMoved(arg)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseMoved(arg)) {
        return _sceneManager->mouseMoved(arg);
    }
    
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonPressed(const Input::MouseEvent& arg,
                                Input::MouseButton button) {
    
    if (Config::Build::ENABLE_EDITOR) {
        if (_platformContext->editor().mouseButtonPressed(arg, button)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseButtonPressed(arg, button)) {
        return _sceneManager->mouseButtonPressed(arg, button);
    }
    
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonReleased(const Input::MouseEvent& arg,
                                 Input::MouseButton button) {
    
    if (Config::Build::ENABLE_EDITOR) {
        if (_platformContext->editor().mouseButtonReleased(arg, button)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseButtonReleased(arg, button)) {
        return _sceneManager->mouseButtonReleased(arg, button);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (!_platformContext->gui().joystickAxisMoved(arg, axis)) {
        return _sceneManager->joystickAxisMoved(arg, axis);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (!_platformContext->gui().joystickPovMoved(arg, pov)) {
        return _sceneManager->joystickPovMoved(arg, pov);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonPressed(const Input::JoystickEvent& arg,
                                   Input::JoystickButton button) {
    if (!_platformContext->gui().joystickButtonPressed(arg, button)) {
        return _sceneManager->joystickButtonPressed(arg, button);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonReleased(const Input::JoystickEvent& arg,
                                    Input::JoystickButton button) {
    if (!_platformContext->gui().joystickButtonReleased(arg, button)) {
        return _sceneManager->joystickButtonReleased(arg, button);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    if (!_platformContext->gui().joystickSliderMoved(arg, index)) {
        return _sceneManager->joystickSliderMoved(arg, index);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickvector3Moved(const Input::JoystickEvent& arg, I8 index) {
    if (!_platformContext->gui().joystickvector3Moved(arg, index)) {
        return _sceneManager->joystickvector3Moved(arg, index);
    }

    // InputInterface needs to know when this is completed
    return false;
}

};
