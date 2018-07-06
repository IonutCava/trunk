#include "Headers/Application.h"

#include "Headers/Kernel.h"
#include "Headers/ApplicationTimer.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

#if defined(_DEBUG)
#include "Utility/Headers/MemoryTracker.h"
#endif

namespace Divide {

#if defined(_DEBUG)
bool MemoryManager::MemoryTracker::Ready = false;
bool MemoryManager::MemoryTracker::LogAllAllocations = false;
MemoryManager::MemoryTracker MemoryManager::AllocTracer;
#endif

Application::Application() : _kernel(nullptr)
{

#if defined(_DEBUG)
    MemoryManager::MemoryTracker::Ready = true; //< faster way of disabling memory tracking
    MemoryManager::MemoryTracker::LogAllAllocations = true;
#endif
    _requestShutdown = false;
    _mainLoopPaused = false;
    _mainLoopActive = false;
    _threadID = std::this_thread::get_id();
    _errorCode = ErrorCode::NO_ERR;
    ParamHandler::createInstance();
    Time::ApplicationTimer::createInstance();
    // Don't log parameter requests
    ParamHandler::instance().setDebugOutput(false);
    // Print a copyright notice in the log file
    Console::printCopyrightNotice();
    Console::toggleTimeStamps(true);
    Console::togglethreadID(true);
}

Application::~Application()
{
    for (DELEGATE_CBK<>& cbk : _shutdownCallback) {
        cbk();
    }
    Console::printfn(Locale::get(_ID("STOP_APPLICATION")));

#if defined(_DEBUG)
    MemoryManager::MemoryTracker::Ready = false;
    bool leakDetected = false;
    size_t sizeLeaked = 0;
    stringImpl allocLog =
        MemoryManager::AllocTracer.Dump(leakDetected, sizeLeaked);
    if (leakDetected) {
        Console::errorfn(Locale::get(_ID("ERROR_MEMORY_NEW_DELETE_MISMATCH")),
                         to_int(std::ceil(sizeLeaked / 1024.0f)));
    }
    std::ofstream memLog;
    memLog.open(_memLogBuffer);
    memLog << allocLog;
    memLog.close();
#endif

    _windowManager.close();
    ParamHandler::destroyInstance();
    Time::ApplicationTimer::destroyInstance();
    _kernel.reset(nullptr);
    Locale::clear();
    Console::stop();
}

ErrorCode Application::initialize(const stringImpl& entryPoint, I32 argc, char** argv) {
    assert(!entryPoint.empty());
    Console::start();
    // Target FPS is usually 60. So all movement is capped around that value
    Time::ApplicationTimer::instance().init(Config::TARGET_FRAME_RATE);
    // Read language table
    if (!Locale::init()) {
        return errorCode();
    }
    Console::printfn(Locale::get(_ID("START_APPLICATION")));

    // Create a new kernel
    _kernel.reset(new Kernel(argc, argv, this->instance()));
    assert(_kernel.get() != nullptr);

    // and load it via an XML file config
    ErrorCode err = Attorney::KernelApplication::initialize(*_kernel, entryPoint);
    if (err != ErrorCode::NO_ERR) {
        _kernel.reset(nullptr);
    }

    return err;
}

void Application::run() {
    Kernel& kernel = *_kernel;
    //Make sure we are displaying a splash screen
    _windowManager.getActiveWindow().type(WindowType::SPLASH);
    Attorney::KernelApplication::warmup(kernel);
    //Restore to normal window
    _windowManager.getActiveWindow().previousType();

    Console::printfn(Locale::get(_ID("START_MAIN_LOOP")));

    mainLoopActive(true);
    while(onLoop()) {
        Attorney::KernelApplication::onLoop(kernel);
    }

    Attorney::KernelApplication::shutdown(kernel);
}

bool Application::onLoop() {
    _windowManager.handleWindowEvent(WindowEvent::APP_LOOP, -1, -1, -1);
    return mainLoopActive();
}

void Application::setCursorPosition(I32 x, I32 y) const {
    _windowManager.setCursorPosition(x, y);
    Attorney::KernelApplication::setCursorPosition(*_kernel, x, y);
}

void Application::onChangeWindowSize(U16 w, U16 h) const {
    Attorney::KernelApplication::onChangeWindowSize(*_kernel, w, h);
}

void Application::onChangeRenderResolution(U16 w, U16 h) const {
    Attorney::KernelApplication::onChangeRenderResolution(*_kernel, w, h);
}

}; //namespace Divide
