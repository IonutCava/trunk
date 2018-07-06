#include "Headers/Application.h"

#include "Headers/Kernel.h"
#include "Headers/ParamHandler.h"

#include "Core/Time/Headers/ApplicationTimer.h"

#if defined(_DEBUG)
#include "Utility/Headers/MemoryTracker.h"
#endif

#include <thread>

namespace Divide {

#if defined(_DEBUG)
bool MemoryManager::MemoryTracker::Ready = false;
bool MemoryManager::MemoryTracker::LogAllAllocations = false;
MemoryManager::MemoryTracker MemoryManager::AllocTracer;
#endif

bool Application::initStaticData() {
#if defined(_DEBUG)
    MemoryManager::MemoryTracker::Ready = true; //< faster way of disabling memory tracking
    MemoryManager::MemoryTracker::LogAllAllocations = true;
#endif
    
    return Kernel::initStaticData();
}

bool Application::destroyStaticData() {
    return Kernel::destroyStaticData();
}

Application::Application() : _kernel(nullptr)
{
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
    memLog.open(_memLogBuffer.c_str());
    memLog << allocLog;
    memLog.close();
#endif

    _windowManager.close();
    ParamHandler::destroyInstance();
    Time::ApplicationTimer::destroyInstance();
    MemoryManager::DELETE(_kernel);
    Locale::clear();
    Console::stop();
}

ErrorCode Application::initialize(const stringImpl& entryPoint, I32 argc, char** argv) {
    assert(!entryPoint.empty());
    Console::start();
    // Read language table
    if (!Locale::init()) {
        return errorCode();
    }
    Console::printfn(Locale::get(_ID("START_APPLICATION")));

    // Create a new kernel
    assert(_kernel == nullptr);
    _kernel = MemoryManager_NEW Kernel(argc, argv, this->instance());

    // and load it via an XML file config
    ErrorCode err = Attorney::KernelApplication::initialize(*_kernel, entryPoint);
    if (err != ErrorCode::NO_ERR) {
        MemoryManager::DELETE(_kernel);
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

    {
        UpgradableReadLock ur_lock(_taskLock);
        bool isQueueEmpty = _mainThreadCallbacks.empty();
        if (!isQueueEmpty) {
            UpgradeToWriteLock w_lock(ur_lock);
            while(!_mainThreadCallbacks.empty()) {
                _mainThreadCallbacks.back()();
                _mainThreadCallbacks.pop_back();
            }
        }
    }

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

std::thread::id Application::mainThreadID() const {
    return _threadID;
}

bool Application::isMainThread() const {
    return (_threadID == std::this_thread::get_id());
}

void Application::mainThreadTask(const DELEGATE_CBK<>& task, bool wait) {
    std::atomic_bool done = false;
    if (wait) {
        WriteLock w_lock(_taskLock);
        _mainThreadCallbacks.push_back([&done, &task] { task(); done = true; });
    } else {
        WriteLock w_lock(_taskLock);
        _mainThreadCallbacks.push_back(task);
    }

    if (wait) {
        WAIT_FOR_CONDITION(done);
    }
}

void Attorney::ApplicationTask::syncThreadToGPU(std::thread::id threadID, bool beginSync) {
    Attorney::KernelApplication::syncThreadToGPU(*Application::instance()._kernel, threadID, beginSync);
}


}; //namespace Divide
