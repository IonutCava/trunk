#include "stdafx.h"

#include "config.h"

#include "Headers/PlatformDefines.h"
#include "Headers/PlatformRuntime.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIMessageBox.h"

#include "Core/Headers/Console.h"

#include <Allocator/xallocator.h>

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
    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        if (MemoryTracker::Ready) {
             AllocTracer.Add( p, size, zFile, nLine );
        }
    }
}

void log_delete(void* p) {
    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
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
}

SysInfo& sysInfo() noexcept {
    return g_sysInfo;
}

const SysInfo& const_sysInfo() noexcept {
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

U32 HARDWARE_THREAD_COUNT() noexcept {
    return std::max(std::thread::hardware_concurrency(), 2u);
}


bool createDirectories(const char* path) {
    assert(path != nullptr && strlen(path) > 0);
    //Always end in a '/'
    assert(path[strlen(path) - 1] == '/');

    vectorSTD<stringImpl> directories;
    Util::Split<vectorSTD<stringImpl>, stringImpl>(path, '/', directories);
    if (directories.empty()) {
        Util::Split<vectorSTD<stringImpl>, stringImpl>(path, '\\', directories);
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
    if (argv0 == nullptr || argv0[0] == 0) {
        return FileWithPath();
    }

    return splitPathToNameAndLocation(extractFilePathAndName(argv0).c_str());
}

const char* GetClipboardText(void* user_data) noexcept
{
    ACKNOWLEDGE_UNUSED(user_data);

    return SDL_GetClipboardText();
}

void SetClipboardText(void* user_data, const char* text) noexcept
{
    ACKNOWLEDGE_UNUSED(user_data);

    SDL_SetClipboardText(text);
}

void ToggleCursor(bool state) noexcept
{
    SDL_ShowCursor((state ? SDL_TRUE : SDL_FALSE));
}

bool CursorState() noexcept
{
    return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
}

};  // namespace Divide

void* operator new(size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
#if defined(_DEBUG)
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
#else
    ACKNOWLEDGE_UNUSED(zFile);
    ACKNOWLEDGE_UNUSED(nLine);
#endif
    return ptr;
}

void operator delete(void* ptr, const char* zFile, size_t nLine) {
#if defined(_DEBUG)
    Divide::MemoryManager::log_delete(ptr);
#else
    ACKNOWLEDGE_UNUSED(zFile);
    ACKNOWLEDGE_UNUSED(nLine);
#endif
    free(ptr);
}

void* operator new[](size_t size, const char* zFile, size_t nLine) {
    void* ptr = malloc(size);
#if defined(_DEBUG)
    Divide::MemoryManager::log_new(ptr, size, zFile, nLine);
#else
ACKNOWLEDGE_UNUSED(zFile);
ACKNOWLEDGE_UNUSED(nLine);
#endif
    return ptr;
}

void operator delete[](void* ptr, const char* zFile, size_t nLine) {
#if defined(_DEBUG)
    Divide::MemoryManager::log_delete(ptr);
#else
ACKNOWLEDGE_UNUSED(zFile);
ACKNOWLEDGE_UNUSED(nLine);
#endif
    free(ptr);
}
