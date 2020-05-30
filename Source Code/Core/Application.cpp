#include "stdafx.h"

#include "Headers/Application.h"

#include "Headers/Kernel.h"
#include "Headers/ParamHandler.h"

#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Platform/File/Headers/FileManagement.h"

#include "Utility/Headers/MemoryTracker.h"

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

    if_constexpr (Config::Build::IS_DEBUG_BUILD) {
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
    _timer.reset();

    Console::toggleImmediateMode(true);
    Console::printfn(Locale::get(_ID("START_APPLICATION")));
    for (U8 i = 1; i < argc; ++i) {
        Console::printfn(Locale::get(_ID("START_APPLICATION_CMD_ARGUMENTS")));
        Console::printfn(" -- %s", argv[i]);
    }
    // Create a new kernel
    assert(_kernel == nullptr);
    _kernel = MemoryManager_NEW Kernel(argc, argv, *this);

    // and load it via an XML file config
    err = Attorney::KernelApplication::initialize(_kernel, entryPoint);

    // failed to start, so cleanup
    if (err != ErrorCode::NO_ERR) {
        throwError(err);
        stop();
    } else {
        Attorney::KernelApplication::warmup(_kernel);
        Console::printfn(Locale::get(_ID("START_MAIN_LOOP")));
        Console::toggleImmediateMode(false);
        mainLoopActive(true);
    }

    return err;
}

void Application::stop() {
    if (_isInitialized) {
        if (_kernel != nullptr) {
            Attorney::KernelApplication::shutdown(_kernel);
        }
        for (DELEGATE<void>& cbk : _shutdownCallback) {
            cbk();
        }

        _windowManager.close();
        MemoryManager::DELETE(_kernel);
        Console::printfn(Locale::get(_ID("STOP_APPLICATION")));
        _isInitialized = false;

        if_constexpr(Config::Build::IS_DEBUG_BUILD) {
            MemoryManager::MemoryTracker::Ready = false;
            bool leakDetected = false;
            size_t sizeLeaked = 0;
            stringImpl allocLog = MemoryManager::AllocTracer.Dump(leakDetected, sizeLeaked);
            if (leakDetected) {
                Console::errorfn(Locale::get(_ID("ERROR_MEMORY_NEW_DELETE_MISMATCH")), to_I32(std::ceil(sizeLeaked / 1024.0f)));
            }
            std::ofstream memLog;
            memLog.open((Paths::g_logPath + _memLogBuffer).c_str());
            memLog << allocLog;
            memLog.close();
        }


        OPTICK_SHUTDOWN();
    }
}

bool Application::step() {
    if (onLoop()) {
        OPTICK_FRAME("MainThread");
        Attorney::KernelApplication::onLoop(_kernel);
        return true;
    }
    windowManager().hideAll();

    return false;
}

bool Application::onLoop() {
    UniqueLock<Mutex> r_lock(_taskLock);
    if (!_mainThreadCallbacks.empty()) {
        while(!_mainThreadCallbacks.empty()) {
            _mainThreadCallbacks.back()();
            _mainThreadCallbacks.pop_back();
        }
    }


    return mainLoopActive();
}


bool Application::onSDLEvent(SDL_Event event) {
    if (event.type == SDL_QUIT) {
        RequestShutdown();
        return true;
    }

    return false;
}

bool Application::onSizeChange(const SizeChangeParams& params) const {
    return Attorney::KernelApplication::onSizeChange(_kernel, params);
}

void Application::mainThreadTask(const DELEGATE<void>& task, bool wait) {
    std::atomic_bool done = false;
    if (wait) {
        UniqueLock<Mutex> w_lock(_taskLock);
        _mainThreadCallbacks.push_back([&done, &task] { task(); done = true; });
    } else {
        UniqueLock<Mutex> w_lock(_taskLock);
        _mainThreadCallbacks.push_back(task);
    }

    if (wait) {
        WAIT_FOR_CONDITION(done);
    }
}

}; //namespace Divide
