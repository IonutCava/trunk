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


#include "Core/Math/Headers/MathClasses.h"
#include "Hardware/Platform/Headers/Thread.h"

#include <fstream>

namespace Divide {

class Kernel;
enum ErrorCode;
///Lightweight singleton class that manages our application's kernel and window information
DEFINE_SINGLETON( Application )

public:
    ///Startup and shutdown
    ErrorCode initialize(const stringImpl& entryPoint,I32 argc, char **argv);
    void deinitialize();
    void run();

    ///Application resolution (either fullscreen resolution or window dimensions)
    inline const vec2<U16>& getResolution()   const {return _resolution;}
    inline const vec2<U16>& getScreenCenter() const {return _screenCenter;}
    inline const vec2<U16>& getPreviousResolution() const { return _prevResolution; }
    inline void setResolutionWidth(U16 w)           { _prevResolution.set(_resolution); _resolution.width = w; _screenCenter.x = w / 2;}
    inline void setResolutionHeight(U16 h)          { _prevResolution.set(_resolution); _resolution.height = h; _screenCenter.y = h / 2;}
    inline void setResolution(U16 w, U16 h)         { _prevResolution.set(_resolution); _resolution.set(w,h); _screenCenter.set(_resolution / 2);}

    inline void RequestShutdown()                   { _requestShutdown = true;  }
    inline void CancelShutdown()                    { _requestShutdown = false; }
    inline bool ShutdownRequested()           const { return _requestShutdown;  }
    inline Kernel* const getKernel()          const { return _kernel; }

    inline const std::thread::id&  getMainThreadId()          const { return _threadId; }
    inline bool isMainThread()                                const { return (_threadId == std::this_thread::get_id()); }
    inline void setMemoryLogFile(const stringImpl& fileName)        { _memLogBuffer = fileName; }

    inline bool hasFocus()                 const { return _hasFocus; }
    inline void hasFocus(const bool state)       { _hasFocus = state; }

    inline bool isFullScreen()                 const { return _isFullscreen;  }
    inline void isFullScreen(const bool state)       { _isFullscreen = state; }

    inline bool mainLoopActive()           const { return _mainLoopActive;  }
    inline void mainLoopActive(bool state)       { _mainLoopActive = state; }

    inline bool mainLoopPaused()           const { return _mainLoopPaused;  }
    inline void mainLoopPaused(bool state)       { _mainLoopPaused = state; }

    void snapCursorToPosition(U16 x, U16 y) const;

    inline void snapCursorToCenter() const {
        snapCursorToPosition(_screenCenter.x, _screenCenter.y);
    }

    inline void      throwError(ErrorCode err)       { _errorCode = err; }
    inline ErrorCode errorCode()               const { return _errorCode; }

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
    ///buffer to register all of the memory allocations recorded via "New"
    stringImpl _memLogBuffer;
    ///Main application thread id
    std::thread::id _threadId;

END_SINGLETON

}; //namespace Divide

#endif