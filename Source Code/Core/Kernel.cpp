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
#include "Platform/Headers/SDLEventManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Managers/Headers/FrameListenerManager.h"
#include "Platform/File/Headers/FileWatcherManager.h"
#include "Platform/Compute/Headers/OpenCLInterface.h"

namespace Divide {

namespace {
    constexpr U32 g_printTimerBase = 15u;
    U32 g_printTimer = g_printTimerBase;
};

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
      _prevViewport(-1),
      _prevPlayerCount(0),
      _renderPassManager(nullptr),
      _splashScreenUpdating(false),
      _platformContext(std::make_unique<PlatformContext>(parentApp, *this)),
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

    FrameListenerManager::instance();
    //OpenCLInterface::instance();
}

Kernel::~Kernel()
{
}

void Kernel::startSplashScreen() {
    bool expected = false;
    if (!_splashScreenUpdating.compare_exchange_strong(expected, true)) {
        return;
    }

    DisplayWindow& window = _platformContext->activeWindow();
    window.changeType(WindowType::WINDOW);
    window.decorated(false);
    WAIT_FOR_CONDITION(window.setDimensions(_platformContext->config().runtime.splashScreenSize));
    window.centerWindowPosition();
    window.hidden(false);
    SDLEventManager::pollEvents();

    GUISplash splash(*_resCache, "divideLogo.jpg", _platformContext->config().runtime.splashScreenSize);

    // Load and render the splash screen
    _splashTask = CreateTask(*_platformContext,
        [this, &splash](const Task& /*task*/) {
        U64 previousTimeUS = 0;
        U64 currentTimeUS = Time::ElapsedMicroseconds(true);
        while (_splashScreenUpdating) {
            U64 deltaTimeUS = currentTimeUS - previousTimeUS;
            previousTimeUS = currentTimeUS;
            _platformContext->beginFrame(PlatformContext::ComponentType::GFXDevice);
            splash.render(_platformContext->gfx(), deltaTimeUS);
            _platformContext->endFrame(PlatformContext::ComponentType::GFXDevice);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));

            break;
        }
    });
    _splashTask.startTask(TaskPriority::REALTIME/*HIGH*/);

    window.swapBuffers(false);
}

void Kernel::stopSplashScreen() {
    DisplayWindow& window = _platformContext->activeWindow();
    window.swapBuffers(true);
    vec2<U16> previousDimensions = window.getPreviousDimensions();
    _splashScreenUpdating = false;
    _splashTask.wait();

    window.changeToPreviousType();
    window.decorated(true);
    WAIT_FOR_CONDITION(window.setDimensions(previousDimensions));
    window.setPosition(vec2<I32>(-1));
    SDLEventManager::pollEvents();
}

void Kernel::idle() {
    _platformContext->idle();
    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcherManager::idle();
    }
    _sceneManager->idle();
    Locale::idle();
    Script::idle();

    if (--g_printTimer == 0) {
        Console::printAll();
        g_printTimer = g_printTimerBase;
    }
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
        sceneManager().saveActiveScene(true, false);
        return;
    }

    // Update internal timer
    Time::ApplicationTimer::instance().update();
    {
        Time::ScopedTimer timer(_appLoopTimer);
   
        // Update time at every render loop
        _timingData.update(Time::ElapsedMicroseconds());
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
                deltaTimeUS = _platformContext->config().runtime.useFixedTimestep
                                    ? Time::SecondsToMicroseconds(1) / TICKS_PER_SECOND
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
    // Should equate to approximately once every 10 seconds
    if (platformContext().debug().enabled() && frameCount % (Config::TARGET_FRAME_RATE * Time::Seconds(10)) == 0) {
        Console::printfn(platformContext().debug().output().c_str());
    }

    if (frameCount % (Config::TARGET_FRAME_RATE / 4) == 0) {
        _platformContext->gui().modifyText(_ID("ProfileData"), platformContext().debug().output());
    }

    // Cap FPS
    I16 frameLimit = _platformContext->config().runtime.frameRateLimit;
    F32 deltaMilliseconds = Time::MicrosecondsToMilliseconds<F32>(_timingData.currentTimeDeltaUS());
    F32 targetFrametime = 1000.0f / frameLimit;

    if (deltaMilliseconds < targetFrametime) {
        {
            Time::ScopedTimer timer2(_appIdleTimer);
            idle();
        }

        if (frameLimit > 0) {
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

    if (_platformContext->activeWindow().minimized()) {
        idle();
        return true;
    }

    {
        Time::ScopedTimer timer2(_physicsProcessTimer);
        // Process physics
        _platformContext->pfx().process(realTime);
    }

    bool fixedTimestep = _platformContext->config().runtime.useFixedTimestep;
    {
        Time::ScopedTimer timer2(_sceneUpdateTimer);

        U8 playerCount = _sceneManager->getActivePlayerCount();

        U8 loopCount = 0;
        while (_timingData.runUpdateLoop()) {

            if (loopCount == 0) {
                _sceneUpdateLoopTimer.start();
            }
            _sceneManager->onStartUpdateLoop(loopCount);

            _sceneManager->processGUI(deltaTimeUS);

            // Flush any pending threaded callbacks
            for (U32 i = 0; i < to_U32(TaskPoolType::COUNT); ++i) {
                _platformContext->taskPool(static_cast<TaskPoolType>(i)).flushCallbackQueue();
            }

            // Update scene based on input
            for (U8 i = 0; i < playerCount; ++i) {
                _sceneManager->processInput(i, deltaTimeUS);
            }
            // process all scene events
            _sceneManager->processTasks(deltaTimeUS);
            // Update the scene state based on current time (e.g. animation matrices)
            _sceneManager->updateSceneState(deltaTimeUS);
            // Update visual effect timers as well
            _platformContext->gfx().postFX().update(deltaTimeUS);

            if (loopCount == 0) {
                _sceneUpdateLoopTimer.stop();
            }

            _timingData.endUpdateLoop(deltaTimeUS, fixedTimestep);
            ++loopCount;
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
    if (fixedTimestep && !_timingData.freezeTime()) {
        interpolationFactor = static_cast<D64>(_timingData.currentTimeUS() + deltaTimeUS - _timingData.nextGameTickUS()) / deltaTimeUS;
        CLAMP_01(interpolationFactor);
    }

    GFXDevice::setFrameInterpolationFactor(interpolationFactor);
    
    // Update windows and get input events
    SDLEventManager::pollEvents();

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

void computeViewports(const Rect<I32>& mainViewport, vector<Rect<I32>>& targetViewports, U8 count) {
    
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

    typedef vector<Rect<I32>> ViewportRow;
    typedef vector<ViewportRow> ViewportRows;
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

Time::ProfileTimer& getTimer(Time::ProfileTimer& parentTimer, vector<Time::ProfileTimer*>& timers, U8 index, const char* name) {
    while (timers.size() < index + 1) {
        timers.push_back(&Time::ADD_TIMER(Util::StringFormat("%s %d", name, timers.size()).c_str()));
        parentTimer.addChildTimer(*timers.back());
    }

    return *timers[index];
}

bool Kernel::presentToScreen(FrameEvent& evt, const U64 deltaTimeUS) {
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

    U8 playerCount = _sceneManager->getActivePlayerCount();

    Rect<I32> mainViewport = _platformContext->activeWindow().renderingViewport();
    
    if (_prevViewport != mainViewport || _prevPlayerCount != playerCount) {
        computeViewports(mainViewport, _targetViewports, playerCount);
        _prevViewport.set(mainViewport);
        _prevPlayerCount = playerCount;
    }

    if (Config::Build::ENABLE_EDITOR && _platformContext->editor().running()) {
        computeViewports(_platformContext->editor().getTargetViewport(), _editorViewports, playerCount);
    }

    for (U8 i = 0; i < playerCount; ++i) {
        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_SCENERENDER_START, evt)) {
            return false;
        }

        Attorney::SceneManagerKernel::currentPlayerPass(*_sceneManager, i);
        {
            Time::ProfileTimer& timer = getTimer(_flushToScreenTimer, _renderTimer, i, "Render Timer");
            Time::ScopedTimer time2(timer);
            _renderPassManager->render(_sceneManager->getActiveScene().renderState(), &timer);
        }

        if (!frameMgr.createAndProcessEvent(Time::ElapsedMicroseconds(true), FrameEventType::FRAME_SCENERENDER_END, evt)) {
            return false;
        }
        {
            Time::ScopedTimer time4(getTimer(_flushToScreenTimer, _blitToDisplayTimer, i, "Blit to screen Timer"));

            GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
            GFX::CommandBuffer& buffer = sBuffer();

            if (Config::Build::ENABLE_EDITOR && _platformContext->editor().running()) {
                Attorney::GFXDeviceKernel::blitToRenderTarget(_platformContext->gfx(), RenderTargetID(RenderTargetUsage::EDITOR), _editorViewports[i], buffer);
            } else {
                Attorney::GFXDeviceKernel::blitToScreen(_platformContext->gfx(), _targetViewports[i], buffer);
            }

            Attorney::GFXDeviceKernel::renderDebugUI(_platformContext->gfx(), _platformContext->activeWindow().windowViewport(), buffer);

            _platformContext->gfx().flushCommandBuffer(buffer);
        }
    }

    for (U32 i = playerCount; i < to_U32(_renderTimer.size()); ++i) {
        Time::ProfileTimer::removeTimer(*_renderTimer[i]);
        Time::ProfileTimer::removeTimer(*_blitToDisplayTimer[i]);
        _renderTimer.erase(std::begin(_renderTimer) + i);
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
    static const U8 warmupLoopCount = 3;

    ParamHandler::instance().setParam(_ID("freezeLoopTime"), true);
    for (U8 i = 0; i < warmupLoopCount; ++i) {
        onLoop();
    }
    ParamHandler::instance().setParam(_ID("freezeLoopTime"), false);

    Attorney::SceneManagerKernel::initPostLoadState(*_sceneManager);

    _timingData.update(Time::ElapsedMicroseconds(true));

    stopSplashScreen();
}

ErrorCode Kernel::initialize(const stringImpl& entryPoint) {
    const SysInfo& systemInfo = const_sysInfo();
    if (Config::REQUIRED_RAM_SIZE > systemInfo._availableRam) {
        return ErrorCode::NOT_ENOUGH_RAM;
    }

    // Load info from XML files
    XMLEntryData& entryData = _platformContext->entryData();
    Configuration& config = _platformContext->config();
    XML::loadFromXML(entryData, entryPoint.c_str());
    XML::loadFromXML(config, (entryData.scriptLocation + "/config.xml").c_str());

    //override from commandline argumenst
    if (Util::findCommandLineArgument(_argc, _argv, "enableGPUMessageGroups")) {
        config.debug.enableDebugMsgGroups = true;
    }

    if (Util::findCommandLineArgument(_argc, _argv, "disableRenderAPIDebugging")) {
        config.debug.enableRenderAPIDebugging = false;
    }

    if (config.runtime.targetRenderingAPI >= to_U8(RenderAPI::COUNT)) {
        config.runtime.targetRenderingAPI = to_U8(RenderAPI::OpenGL);
    }

    _platformContext->pfx().setAPI(PXDevice::PhysicsAPI::PhysX);
    _platformContext->sfx().setAPI(SFXDevice::AudioAPI::SDL);
    _platformContext->gfx().setAPI(static_cast<RenderAPI>(config.runtime.targetRenderingAPI));
    

    ASIO::SET_LOG_FUNCTION([](const char* msg, bool is_error) {
        is_error ? Console::errorfn(msg) : Console::printfn(msg);
    });

    Server::instance().init(((Divide::U16)443), "127.0.0.1", true);

    if (!_platformContext->client().connect(entryData.serverAddress, 443)) {
        _platformContext->client().connect("127.0.0.1", 443);
    }

    Paths::updatePaths(*_platformContext);

    Locale::changeLanguage(config.language.c_str());
    ECS::Initialize();

    // Create mem log file
    const stringImpl& mem = config.debug.memFile;
    _platformContext->app().setMemoryLogFile(mem.compare("none") == 0 ? "mem.log" : mem);
    Console::printfn(Locale::get(_ID("START_RENDER_INTERFACE")));

    WindowManager& winManager = _platformContext->app().windowManager();
    ErrorCode initError = winManager.init(*_platformContext,
                                           vec2<I16>(-1),
                                           config.runtime.windowSize,
                                           static_cast<WindowMode>(config.runtime.windowedMode),
                                           config.runtime.targetDisplay);

    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    Camera::initPool();

    initError = _platformContext->gfx().initRenderingAPI(_argc, _argv, config.runtime.resolution);

    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }

    U32 hardwareThreads = config.runtime.maxWorkerThreads < 0 ? 
                          HARDWARE_THREAD_COUNT() :
                          to_U32(config.runtime.maxWorkerThreads) + 2;

    U8 threadCount = static_cast<U8>(std::max(hardwareThreads - 2, to_base(RenderStage::COUNT) * 2u + 2));

    if (!_platformContext->taskPool(TaskPoolType::HIGH_PRIORITY).init(
        threadCount,
        TaskPool::TaskPoolType::TYPE_BLOCKING,
        [this](const std::thread::id& threadID) {
            Attorney::PlatformContextKernel::onThreadCreated(platformContext(), threadID);
        },
        "DIVIDE_WORKER_THREAD_"))
    {
        return ErrorCode::CPU_NOT_SUPPORTED;
    }

    if (!_platformContext->taskPool(TaskPoolType::LOW_PRIORITY).init(
        3,
        TaskPool::TaskPoolType::TYPE_BLOCKING,
        [this](const std::thread::id& threadID) {
            Attorney::PlatformContextKernel::onThreadCreated(platformContext(), threadID);
        },
        "DIVIDE_BACKUP_THREAD_"))
    {
        return ErrorCode::CPU_NOT_SUPPORTED;
    }

    initError = _platformContext->gfx().postInitRenderingAPI();

    // If we could not initialize the graphics device, exit
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }
    /*initError = OpenCLInterface::instance().init();
    if (initError != ErrorCode::NO_ERR) {
        return initError;
    }*/

    // Add our needed app-wide render passes. RenderPassManager is responsible for deleting these!
    _renderPassManager = std::make_unique<RenderPassManager>(*this, _platformContext->gfx());
    _renderPassManager->addRenderPass("shadowPass",     0, RenderStage::SHADOW);
    _renderPassManager->addRenderPass("reflectionPass", 1, RenderStage::REFLECTION);
    _renderPassManager->addRenderPass("refractionPass", 2, RenderStage::REFRACTION);
    _renderPassManager->addRenderPass("displayStage",   3, RenderStage::DISPLAY);

    Console::printfn(Locale::get(_ID("SCENE_ADD_DEFAULT_CAMERA")));

    Script::onStartup();
    SceneManager::onStartup();
    Attorney::ShaderProgramKernel::useShaderTextCache(config.debug.useShaderTextCache);
    Attorney::ShaderProgramKernel::useShaderBinaryCache(config.debug.useShaderBinaryCache);

    const vec2<U16>& drawArea = winManager.getMainWindow().getDrawableSize();
    Rect<U16> targetViewport(0, 0, drawArea.width, drawArea.height);

    // Initialize GUI with our current resolution
    _platformContext->gui().init(*_platformContext, *_resCache);
    startSplashScreen();

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

    _platformContext->gui().addText("ProfileData",                                 // Unique ID
                                    RelativePosition2D(RelativeValue(0.75f, 0.0f),
                                                       RelativeValue(0.2f, 0.0f)), // Position
                                    Font::DROID_SERIF_BOLD,                        // Font
                                    UColour(255,  50, 0, 255),                     // Colour
                                    "PROFILE DATA",                                // Text
                                    12);                                           // Font size

    ShadowMap::initShadowMaps(_platformContext->gfx());
    _sceneManager->init(*_platformContext, *_resCache);

    if (!_sceneManager->switchScene(entryData.startupScene, true, targetViewport, false)) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")), entryData.startupScene.c_str());
        return ErrorCode::MISSING_SCENE_DATA;
    }

    if (!_sceneManager->checkLoadFlag()) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_NOT_CALLED")),
                         entryData.startupScene.c_str());
        return ErrorCode::MISSING_SCENE_LOAD_CALL;
    }
    if (Config::Build::ENABLE_EDITOR) {
        if (!_platformContext->editor().init(config.runtime.resolution)) {
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
    for (U32 i = 0; i < to_U32(TaskPoolType::COUNT); ++i) {
        WaitForAllTasks(_platformContext->taskPool(static_cast<TaskPoolType>(i)), true, true, true);
    }
    
    if (Config::Build::ENABLE_EDITOR) {
        _platformContext->editor().toggle(false);
    }
    SceneManager::onShutdown();
    Script::onShutdown();
    _sceneManager.reset();
    ECS::Terminate();

    ShadowMap::destroyShadowMaps(_platformContext->gfx());
    //OpenCLInterface::instance().deinit();
    _renderPassManager.reset();

    Camera::destroyPool();
    _platformContext->terminate();
    _resCache->clear();

    Console::printfn(Locale::get(_ID("STOP_ENGINE_OK")));
}

void Kernel::onSizeChange(const SizeChangeParams& params) const {

    Attorney::GFXDeviceKernel::onSizeChange(_platformContext->gfx(), params);

    if (!_splashScreenUpdating) {
        _platformContext->gui().onSizeChange(params);
    }

    if (Config::Build::ENABLE_EDITOR) {
        _platformContext->editor().onSizeChange(params);
    }

    if (!params.isWindowResize) {
        _sceneManager->onSizeChange(params);
    }


}

///--------------------------Input Management-------------------------------------///
bool Kernel::setCursorPosition(I32 x, I32 y) {
    _platformContext->gui().setCursorPosition(x, y);
    return true;
}

bool Kernel::onKeyDown(const Input::KeyEvent& key) {
    if (Config::Build::ENABLE_EDITOR) {

        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.onKeyDown(key)) {
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
        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.onKeyUp(key)) {
            return true;
        }
    }

    if (!_platformContext->gui().onKeyUp(key)) {
        return _sceneManager->onKeyUp(key);
    }
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (Config::Build::ENABLE_EDITOR) {
        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.mouseMoved(arg)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseMoved(arg)) {
        return _sceneManager->mouseMoved(arg);
    }
    
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
    
    if (Config::Build::ENABLE_EDITOR) {
        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.mouseButtonPressed(arg)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseButtonPressed(arg)) {
        return _sceneManager->mouseButtonPressed(arg);
    }
    
    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    
    if (Config::Build::ENABLE_EDITOR) {
        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.mouseButtonReleased(arg)) {
            return true;
        }
    }

    if (!_platformContext->gui().mouseButtonReleased(arg)) {
        return _sceneManager->mouseButtonReleased(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickAxisMoved(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickAxisMoved(arg)) {
        return _sceneManager->joystickAxisMoved(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickPovMoved(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickPovMoved(arg)) {
        return _sceneManager->joystickPovMoved(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonPressed(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickButtonPressed(arg)) {
        return _sceneManager->joystickButtonPressed(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickButtonReleased(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickButtonReleased(arg)) {
        return _sceneManager->joystickButtonReleased(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickBallMoved(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickBallMoved(arg)) {
        return _sceneManager->joystickBallMoved(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickAddRemove(const Input::JoystickEvent& arg) {
    if (!_platformContext->gui().joystickAddRemove(arg)) {
        return _sceneManager->joystickAddRemove(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::joystickRemap(const Input::JoystickEvent &arg) {
    if (!_platformContext->gui().joystickRemap(arg)) {
        return _sceneManager->joystickRemap(arg);
    }

    // InputInterface needs to know when this is completed
    return false;
}

bool Kernel::onUTF8(const Input::UTF8Event& arg) {
    if (Config::Build::ENABLE_EDITOR) {
        Editor& editor = _platformContext->editor();
        if (editor.running() && editor.onUTF8(arg)) {
            return true;
        }
    }
    
    if (!_platformContext->gui().onUTF8(arg)) {
        return _sceneManager->onUTF8(arg);
    }

    return false;
}
};

