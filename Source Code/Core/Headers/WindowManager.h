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

#ifndef _CORE_WINDOW_MANAGER_H_
#define _CORE_WINDOW_MANAGER_H_

#include "Platform/Headers/DisplayWindow.h"

namespace Divide {

enum class WindowEvent : U32 {
    HIDDEN = 0,
    SHOWN = 1,
    MINIMIZED = 2,
    MAXIMIZED = 3,
    RESTORED = 4,
    LOST_FOCUS = 5,
    GAINED_FOCUS = 6,
    RESIZED_INTERNAL = 7,
    RESIZED_EXTERNAL = 8,
    RESOLUTION_CHANGED = 9,
    MOVED = 10,
    APP_LOOP = 11
};

enum class RenderAPI : U32;

class PlatformContext;
class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    ErrorCode init(PlatformContext& context,
                   RenderAPI api,
                   ResolutionByType initialResolutions,
                   bool startFullScreen,
                   I32 targetDisplayIndex);

    void close();

    void setActiveWindow(U32 index);
    ErrorCode initWindow(U32 index,
                         U32 windowFlags,
                         const ResolutionByType& initialResolutions,
                         bool startFullScreen,
                         I32 targetDisplayIndex,
                         const char* windowTitle);
    inline I32 targetDisplay() const;
    inline void targetDisplay(I32 displayIndex);

    void setCursorPosition(I32 x, I32 y) const;

    inline DisplayWindow& getActiveWindow();
    inline const DisplayWindow& getActiveWindow() const;
    inline DisplayWindow& getWindow(I64 guid);
    inline const DisplayWindow& getWindow(I64 guid) const;
    void handleWindowEvent(WindowEvent event, I64 winGUID, I32 data1, I32 data2);

protected:
    U32 createAPIFlags(RenderAPI api);

protected:
    I32 _displayIndex;
    I64 _activeWindowGUID;
    PlatformContext* _context;
    vectorImpl<DisplayWindow*> _windows;
};
}; //namespace Divide
#endif //_CORE_WINDOW_MANAGER_H_

#include "WindowManager.inl"