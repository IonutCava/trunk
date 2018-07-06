/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _CORE_APPLICATION_H_
#define _CORE_APPLICATION_H_

#include "WindowManager.h"
#include <thread>

namespace Divide {

enum class ErrorCode : I32 {
    NO_ERR = 0,
    MISSING_SCENE_DATA = -1,
    MISSING_SCENE_LOAD_CALL = -2,
    CPU_NOT_SUPPORTED = -3,
    GFX_NOT_SUPPORTED = -4,
    GFX_NON_SPECIFIED = -5,
    SFX_NON_SPECIFIED = -6,
    PFX_NON_SPECIFIED = -7,
    WINDOW_INIT_ERROR = -8,
    SDL_WINDOW_INIT_ERROR = -9,
    FONT_INIT_ERROR = -10,
    GLBINGING_INIT_ERROR = -11,
    GLSL_INIT_ERROR = -12,
    GL_OLD_HARDWARE = -13,
    DX_INIT_ERROR = -14,
    DX_OLD_HARDWARE = -15,
    OGL_OLD_HARDWARE = -16,
    SDL_AUDIO_INIT_ERROR = -17,
    SDL_AUDIO_MIX_INIT_ERROR = -18,
    FMOD_AUDIO_INIT_ERROR = -19,
    OAL_INIT_ERROR = -20,
    OCL_INIT_ERROR = -21,
    PHYSX_INIT_ERROR = -22,
    PHYSX_EXTENSION_ERROR = -23,
    NO_LANGUAGE_INI = -24,
    NOT_ENOUGH_RAM = -25,
    PLATFORM_INIT_ERROR = -26,
    PLATFORM_CLOSE_ERROR = -27,
};

class Task;
class Kernel;
const char* getErrorCodeName(ErrorCode code);
  
namespace Attorney {
    class ApplicationTask;
};

/// Lightweight singleton class that manages our application's kernel and window
/// information
DEFINE_SINGLETON(Application)
    friend class Attorney::ApplicationTask;
  public:
    static bool             isMainThread();
    static std::thread::id  mainThreadID();

    /// Startup and shutdown
    ErrorCode start(const stringImpl& entryPoint, I32 argc, char** argv);
    void      stop();

    bool step();
    bool onLoop();

    inline void RequestShutdown();
    inline void CancelShutdown();
    inline bool ShutdownRequested() const;

    inline Kernel& kernel() const;
    inline WindowManager& windowManager();
    inline const WindowManager& windowManager() const;

    void mainThreadTask(const DELEGATE_CBK<>& task, bool wait = true);

    inline void setMemoryLogFile(const stringImpl& fileName);

    inline bool mainLoopActive() const;
    inline void mainLoopActive(bool state);

    inline bool mainLoopPaused() const;
    inline void mainLoopPaused(bool state);

    void onChangeWindowSize(U16 w, U16 h) const;
    void onChangeRenderResolution(U16 w, U16 h) const;

    void setCursorPosition(I32 x, I32 y) const;
    inline void snapCursorToCenter() const;
        
    inline void throwError(ErrorCode err);
    inline ErrorCode errorCode() const;

    inline SysInfo& sysInfo();
    inline const SysInfo& sysInfo() const;
    /// Add a list of callback functions that should be called when the application
    /// instance is destroyed
    /// (release hardware, file handlers, etc)
    inline void registerShutdownCallback(const DELEGATE_CBK<>& cbk);

  private:

    Application();
    ~Application();
    static void mainThreadID(std::thread::id threadID);

    //ToDo: Remove this hack - Ionut
    void warmup();
  private:
    SysInfo _sysInfo;
    WindowManager _windowManager;
     
    ErrorCode _errorCode;
    /// this is true when we are inside the main app loop
    std::atomic_bool _mainLoopActive;
    std::atomic_bool _mainLoopPaused;
    std::atomic_bool _requestShutdown;
    bool             _isInitialized;
    Kernel* _kernel;
    /// buffer to register all of the memory allocations recorded via
    /// "MemoryManager_NEW"
    stringImpl _memLogBuffer;
    /// Main application thread ID
    static std::thread::id _threadID;
    /// A list of callback functions that get called when the application instance
    /// is destroyed
    vectorImpl<DELEGATE_CBK<> > _shutdownCallback;

    /// A list of callbacks to execute on the main thread
    mutable SharedLock _taskLock;
    vectorImpl<DELEGATE_CBK<> > _mainThreadCallbacks;
END_SINGLETON

namespace Attorney {
    class ApplicationTask {
    private:
        // threadID = calling thread
        // beginSync = true, called before thread processes data / false, called when thread finished processing data
        static void syncThreadToGPU(std::thread::id threadID, bool beginSync);

        friend class Divide::Task;
    };
};  // namespace Attorney

};  // namespace Divide

#endif  //_CORE_APPLICATION_H_

#include "Application.inl"
