/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _APPLICATION_H_
#define _APPLICATION_H_


#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Threading/Headers/Thread.h"

#include <fstream>

namespace Divide {

enum ErrorCode;

class Kernel;
enum ErrorCode;
const char* getErrorCodeName(ErrorCode code);

///Lightweight singleton class that manages our application's kernel and window information
DEFINE_SINGLETON( Application )

public:
    ///Startup and shutdown
    ErrorCode initialize(const stringImpl& entryPoint,I32 argc, char **argv);
    void deinitialize();
    void run();

    ///Application resolution (either fullscreen resolution or window dimensions)
    inline const vec2<U16>& getResolution()   const;
    inline const vec2<U16>& getScreenCenter() const;
    inline const vec2<U16>& getPreviousResolution() const;

    inline void setResolutionWidth(U16 w);
    inline void setResolutionHeight(U16 h);
    inline void setResolution(U16 w, U16 h);

    inline void RequestShutdown();
    inline void CancelShutdown();
    inline bool ShutdownRequested() const;
    inline Kernel* const getKernel() const;

    inline bool isMainThread() const;
    inline const std::thread::id&  getMainThreadId() const;
    inline void setMemoryLogFile(const stringImpl& fileName);

    inline bool hasFocus() const;
    inline void hasFocus(const bool state);

    inline bool isFullScreen() const;
    inline void isFullScreen(const bool state);

    inline bool mainLoopActive() const;
    inline void mainLoopActive(bool state);

    inline bool mainLoopPaused() const;
    inline void mainLoopPaused(bool state);

    void snapCursorToPosition(U16 x, U16 y) const;

    inline void snapCursorToCenter() const;

    inline void throwError(ErrorCode err);
    inline ErrorCode errorCode() const;

    /// Add a list of callback functions that should be called when the application instance is destroyed
    /// (release hardware, file handlers, etc)
    inline void registerShutdownCallback( const DELEGATE_CBK<>& cbk );

private:
    Application();
    ~Application();

private:
    ErrorCode _errorCode;
    /// this is true when we are inside the main app loop
    std::atomic_bool  _mainLoopActive;
    std::atomic_bool  _mainLoopPaused;
    std::atomic_bool  _requestShutdown;
    /// this is false if the window/application lost focus (e.g. clicked another window, alt + tab, etc)
    bool      _hasFocus; 
    /// this is false if the application is running in windowed mode
    bool      _isFullscreen;
    vec2<U16> _resolution;
    vec2<U16> _screenCenter;
    vec2<U16> _prevResolution;
    Kernel*   _kernel;
    ///buffer to register all of the memory allocations recorded via "MemoryManager_NEW"
    stringImpl _memLogBuffer;
    ///Main application thread id
    std::thread::id _threadId;
    ///A list of callback functions that get called when the application instance is destroyed
    vectorImpl<DELEGATE_CBK<> > _shutdownCallback;
END_SINGLETON

}; //namespace Divide

#include "Application.inl"

#endif