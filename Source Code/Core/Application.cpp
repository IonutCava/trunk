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
MemoryManager::MemoryTracker MemoryManager::AllocTracer;
#endif

Application::Application() : _kernel(nullptr)
{

#if defined(_DEBUG)
    MemoryManager::MemoryTracker::Ready = false; //< faster way of disabling memory tracking
#endif
    _requestShutdown = false;
    _mainLoopActive = false;
    _mainLoopPaused = false;
    _threadID = std::this_thread::get_id();
    _errorCode = ErrorCode::NO_ERR;
    ParamHandler::createInstance();
    Time::ApplicationTimer::createInstance();
    // Don't log parameter requests
    ParamHandler::getInstance().setDebugOutput(false);
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
    Console::flush();
}

ErrorCode Application::initialize(const stringImpl& entryPoint, I32 argc, char** argv) {
    assert(!entryPoint.empty());
    // Target FPS is usually 60. So all movement is capped around that value
    Time::ApplicationTimer::getInstance().init(Config::TARGET_FRAME_RATE);
    // Read language table
    if (!Locale::init()) {
        return errorCode();
    }
    Console::printfn(Locale::get(_ID("START_APPLICATION")));

    // Create a new kernel
    _kernel.reset(new Kernel(argc, argv, this->getInstance()));
    assert(_kernel.get() != nullptr);

    // and load it via an XML file config
    ErrorCode err = _kernel->initialize(entryPoint);
    if (err != ErrorCode::NO_ERR) {
        _kernel.reset(nullptr);
    }

    return err;
}

void Application::run() {
    _kernel->runLogicLoop();
}

void Application::onLoop() {
    _windowManager.handleWindowEvent(WindowEvent::APP_LOOP, -1, -1, -1);
}

void Application::setCursorPosition(I32 x, I32 y) const {
    _windowManager.setCursorPosition(x, y);
    _kernel->setCursorPosition(x, y);
}

void Application::onChangeWindowSize(U16 w, U16 h) const {
    _kernel->onChangeWindowSize(w, h);
}

}; //namespace Divide
