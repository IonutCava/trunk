/*
   Copyright (c) 2017 DIVIDE-Studio
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

namespace Divide {

class Task;
class Kernel;
const char* getErrorCodeName(ErrorCode code);
  
namespace Attorney {
    class ApplicationTask;
};

/// Lightweight singleton class that manages our application's kernel and window
/// information
class Application {
    friend class Attorney::ApplicationTask;
  public:
     Application();
     ~Application();

    /// Startup and shutdown
    ErrorCode start(const stringImpl& entryPoint, I32 argc, char** argv);
    void      stop();

    void idle();

    bool step();
    bool onLoop();

    inline void RequestShutdown();
    inline void CancelShutdown();
    inline bool ShutdownRequested() const;

    inline Kernel& kernel() const;
    inline WindowManager& windowManager();
    inline const WindowManager& windowManager() const;

    void mainThreadTask(const DELEGATE_CBK<void>& task, bool wait = true);

    inline void setMemoryLogFile(const stringImpl& fileName);

    inline bool mainLoopActive() const;
    inline void mainLoopActive(bool state);

    inline bool mainLoopPaused() const;
    inline void mainLoopPaused(bool state);

    void onChangeWindowSize(U16 w, U16 h) const;
    void onChangeRenderResolution(U16 w, U16 h) const;

    inline void throwError(ErrorCode err);
    inline ErrorCode errorCode() const;

    /// Add a list of callback functions that should be called when the application
    /// instance is destroyed
    /// (release hardware, file handlers, etc)
    inline void registerShutdownCallback(const DELEGATE_CBK<void>& cbk);

  private:

    //ToDo: Remove this hack - Ionut
    void warmup();

  private:
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
    /// A list of callback functions that get called when the application instance
    /// is destroyed
    vectorImpl<DELEGATE_CBK<void> > _shutdownCallback;

    /// A list of callbacks to execute on the main thread
    mutable SharedLock _taskLock;
    vectorImpl<DELEGATE_CBK<void> > _mainThreadCallbacks;
};

};  // namespace Divide

#endif  //_CORE_APPLICATION_H_

#include "Application.inl"
