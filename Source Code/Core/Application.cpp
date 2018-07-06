#include "stdafx.h"

#include "Headers/Application.h"

#include "Headers/Kernel.h"
#include "Headers/ParamHandler.h"

#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"

#include "Utility/Headers/MemoryTracker.h"

#include <thread>

namespace Divide {

bool MemoryManager::MemoryTracker::Ready = false;
bool MemoryManager::MemoryTracker::LogAllAllocations = false;
MemoryManager::MemoryTracker MemoryManager::AllocTracer;

Application::Application() : _kernel(nullptr),
                             _isInitialized(false)
{
    _requestShutdown = false;
    _mainLoopPaused = false;
    _mainLoopActive = false;
    _errorCode = ErrorCode::NO_ERR;

    if (Config::Build::IS_DEBUG_BUILD) {
        MemoryManager::MemoryTracker::Ready = true; //< faster way of disabling memory tracking
        MemoryManager::MemoryTracker::LogAllAllocations = false;
    }
}

Application::~Application()
{
    assert(!_isInitialized);
}

ErrorCode Application::start(const stringImpl& entryPoint, I32 argc, char** argv) {
    assert(!entryPoint.empty());

    _isInitialized = true;
    ErrorCode err = ErrorCode::NO_ERR;
    ParamHandler::createInstance();
    Time::ApplicationTimer::createInstance();
    // Don't log parameter requests
    ParamHandler::instance().setDebugOutput(false);

    Console::printfn(Locale::get(_ID("START_APPLICATION")));
    for (U8 i = 1; i < argc; ++i) {
        Console::printfn(Locale::get(_ID("START_APPLICATION_CMD_ARGUMENTS")));
        Console::printfn(" -- %s", argv[i]);
    }
    // Create a new kernel
    assert(_kernel == nullptr);
    _kernel = MemoryManager_NEW Kernel(argc, argv, *this);

    // and load it via an XML file config
    err = Attorney::KernelApplication::initialize(*_kernel, entryPoint);

    // failed to start, so cleanup
    if (err != ErrorCode::NO_ERR) {
        throwError(err);
        stop();
    } else {
        warmup(_kernel->platformContext().config());
        mainLoopActive(true);
    }

    return err;
}

void Application::stop() {
    if (_isInitialized) {
        if (_kernel != nullptr) {
            Attorney::KernelApplication::shutdown(*_kernel);
        }
        for (DELEGATE_CBK<void>& cbk : _shutdownCallback) {
            cbk();
        }

        if (Config::Build::IS_DEBUG_BUILD) {
            MemoryManager::MemoryTracker::Ready = false;
            bool leakDetected = false;
            size_t sizeLeaked = 0;
            stringImpl allocLog =
                MemoryManager::AllocTracer.Dump(leakDetected, sizeLeaked);
            if (leakDetected) {
                Console::errorfn(Locale::get(_ID("ERROR_MEMORY_NEW_DELETE_MISMATCH")),
                    to_I32(std::ceil(sizeLeaked / 1024.0f)));
            }
            std::ofstream memLog;
            memLog.open(_memLogBuffer.c_str());
            memLog << allocLog;
            memLog.close();
        }

        _windowManager.close();
        ParamHandler::destroyInstance();
        MemoryManager::DELETE(_kernel);
        Console::printfn(Locale::get(_ID("STOP_APPLICATION")));
        Time::ApplicationTimer::destroyInstance();
        _isInitialized = false;
    }
}

void Application::warmup(const Configuration& config) {
    DisplayWindow& window = _windowManager.getActiveWindow();

    vec2<U16> previousDimensions = window.getPreviousDimensions();
    Console::printfn(Locale::get(_ID("START_MAIN_LOOP")));
    //Make sure we are displaying a splash screen
    window.setDimensions(config.runtime.splashScreen.w, config.runtime.splashScreen.h);
    window.changeType(WindowType::SPLASH);
    Attorney::KernelApplication::startSplashScreen(*_kernel);
    window.swapBuffers(false);
    Attorney::KernelApplication::warmup(*_kernel);
    //Restore to normal window
    window.swapBuffers(true);
    Attorney::KernelApplication::stopSplashScreen(*_kernel);
    window.setDimensions(previousDimensions);
    window.changeToPreviousType();
}

void Application::idle() {

}

bool Application::step() {
    if (onLoop()) {
        Attorney::KernelApplication::onLoop(*_kernel);
        return true;
    }

    return false;
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

void Application::onChangeWindowSize(U16 w, U16 h) const {
    Attorney::KernelApplication::onChangeWindowSize(*_kernel, w, h);
}

void Application::onChangeRenderResolution(U16 w, U16 h) const {
    Attorney::KernelApplication::onChangeRenderResolution(*_kernel, w, h);
}

void Application::mainThreadTask(const DELEGATE_CBK<void>& task, bool wait) {
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

}; //namespace Divide
