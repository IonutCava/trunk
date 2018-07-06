#include "stdafx.h"

#include "config.h"

#include "Headers/PlatformDefines.h"
#include "Headers/PlatformRuntime.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
#include <Allocator/xallocator.h>
#endif

#include "Utility/Headers/Localization.h"
#include "Utility/Headers/MemoryTracker.h"

#include "Platform/File/Headers/FileManagement.h"

#define HAVE_M_PI
#include <SDL.h>

namespace Divide {

namespace {
    SysInfo g_sysInfo;
};

namespace MemoryManager {
void log_new(void* p, size_t size, const char* zFile, size_t nLine) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (MemoryTracker::Ready) {
             AllocTracer.Add( p, size, zFile, nLine );
        }
    }
}

void log_delete(void* p) {
    if (Config::Build::IS_DEBUG_BUILD) {
        if (MemoryTracker::Ready) {
            AllocTracer.Remove( p );
        }
    }
}
};  // namespace MemoryManager

SysInfo::SysInfo() : _availableRam(0),
                     _systemResolutionWidth(0),
                     _systemResolutionHeight(0)
{
    _focusedWindowHandle = std::make_unique<WindowHandle>();
}

SysInfo& sysInfo() {
    return g_sysInfo;
}

const SysInfo& const_sysInfo() {
    return g_sysInfo;
}


ErrorCode PlatformPreInit(int argc, char** argv) {
    InitSysInfo(sysInfo(), argc, argv);
    return ErrorCode::NO_ERR;
}

ErrorCode PlatformPostInit(int argc, char** argv) {
    Runtime::mainThreadID(std::this_thread::get_id());
    SeedRandom();
    Paths::initPaths(sysInfo());

    ErrorCode err = ErrorCode::NO_ERR;
    if (pathExists((Paths::g_exePath + Paths::g_assetsLocation).c_str())) {
        // Read language table
        err = Locale::init();
        if (err == ErrorCode::NO_ERR) {
            Console::start();
            // Print a copyright notice in the log file
            if (!Util::findCommandLineArgument(argc, argv, "disableCopyright")) {
                Console::printCopyrightNotice();
            }
            Console::toggleTimeStamps(true);
            Console::togglethreadID(true);
        }
    } else {
        err = ErrorCode::WRONG_WORKING_DIRECTORY;
    }

    return err;
}


ErrorCode PlatformInit(int argc, char** argv) {
    ErrorCode err = ErrorCode::NO_ERR;
    err = PlatformPreInit(argc, argv);
    if (err == ErrorCode::NO_ERR) {
        err = PlatformInitImpl(argc, argv);
        if (err == ErrorCode::NO_ERR) {
            err = PlatformPostInit(argc, argv);
        }
    }

    return err;
}

bool PlatformClose() {
    Runtime::resetMainThreadID();
    if (PlatformCloseImpl()) {
        Console::stop();
        Locale::clear();
        return true;
    }

    return false;
}

void InitSysInfo(SysInfo& info, I32 argc, char** argv) {
    GetAvailableMemory(info);
    info._pathAndFilename = getExecutableLocation(argc, argv);
    info._pathAndFilename._path.append("/");
}

U32 HARDWARE_THREAD_COUNT() {
    return std::max(std::thread::hardware_concurrency(), 2u);
}

extern void DIVIDE_ASSERT_MSG_BOX(const char* failMessage);

bool preAssert(const bool expression, const char* failMessage) {
    if (expression) {
        return false;
    }
    if (Config::Assert::LOG_ASSERTS) {
        Console::errorfn("Assert: %s", failMessage);
    }
    /// Message boxes without continue on assert don't render!
    if (Config::Assert::SHOW_MESSAGE_BOX && Config::Assert::CONTINUE_ON_ASSERT) {
        DIVIDE_ASSERT_MSG_BOX(failMessage);
    }

    return !Config::Assert::CONTINUE_ON_ASSERT;
}

bool createDirectories(const char* path) {
    assert(path != nullptr && strlen(path) > 0);
    //Always end in a '/'
    assert(path[strlen(path) - 1] == '/');

    vector<stringImpl> directories = Util::Split(path, '/');
    if (directories.empty()) {
        directories = Util::Split(path, '\\');
    }

    stringImpl previousPath = ".";
    for (const stringImpl& dir : directories) {
        if (!createDirectory((previousPath + "/" + dir).c_str())) {
            return false;
        }
        previousPath += "/" + dir;
    }

    return true;
}

FileWithPath getExecutableLocation(char* argv0) {
    if (argv0 == nullptr || argv0[0] == 0)
    {
        return FileWithPath();
    }

    boost::system::error_code ec;
    boost::filesystem::path p(boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    return splitPathToNameAndLocation(p.make_preferred().string());
}

const char* GetClipboardText(void* user_data)
{
    ACKNOWLEDGE_UNUSED(user_data);

    return SDL_GetClipboardText();
}

void SetClipboardText(void* user_data, const char* text)
{
    ACKNOWLEDGE_UNUSED(user_data);

    SDL_SetClipboardText(text);
}

void ToggleCursor(bool state)
{
    SDL_ShowCursor(state ? 1 : 0);
}

};  // namespace Divide

#if defined(_DEBUG)
#if defined(DEBUG_EXTERNAL_ALLOCATIONS)
void* operator new(size_t size) {
    static thread_local bool logged = false;
    void* ptr = malloc(size);
    if (!logged) {
        Divide::MemoryManager::log_new(ptr, size, " allocation outside of macro ", 0);
        if (Divide::MemoryManager::MemoryTracker::Ready) {
            logged = true;
        }
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size) {
    static thread_local bool logged = false;
    void* ptr = malloc(size);
    if (!logged) {
        Divide::MemoryManager::log_new(ptr, size, " array allocation outside of macro ", 0);
        if (Divide::MemoryManager::MemoryTracker::Ready) {
            logged = true;
        }
    }
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#endif

void* operator new(size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete(void* ptr, const char* zFile, size_t nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}

void* operator new[](size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
    return ptr;
}

void operator delete[](void* ptr, const char* zFile, size_t nLine) {
    Divide::MemoryManager::log_delete(ptr);
    free(ptr);
}
#else
#endif