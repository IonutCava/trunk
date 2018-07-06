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

#ifndef _CORE_WINDOW_MANAGER_H_
#define _CORE_WINDOW_MANAGER_H_

#include "Platform/Headers/DisplayWindow.h"

namespace Divide {

enum class RenderAPI : U32;

struct WindowDescriptor {
    U32 targetDisplay = 0;
    stringImpl title;
    vec2<U16> dimensions;
    bool fullscreen = false;
};

class PlatformContext;
class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    ErrorCode init(PlatformContext& context,
                   RenderAPI api,
                   const vec2<U16>& initialResolution,
                   bool startFullScreen,
                   I32 targetDisplayIndex);

    void close();

    void update(const U64 deltaTime);

    bool anyWindowFocus() const;

    void setActiveWindow(U32 index);
    U32 createWindow(const WindowDescriptor& descriptor, ErrorCode& err);
    bool destroyWindow(DisplayWindow*& window);

    inline I32 targetDisplay() const;
    inline void targetDisplay(I32 displayIndex);

    void setCursorPosition(I32 x, I32 y) const;
    vec2<I32> getCursorPosition() const;
    void snapCursorToCenter() const;

    inline DisplayWindow& getActiveWindow();
    inline const DisplayWindow& getActiveWindow() const;
    inline DisplayWindow& getWindow(I64 guid);
    inline const DisplayWindow& getWindow(I64 guid) const;
    inline DisplayWindow& getWindow(U32 index);
    inline const DisplayWindow& getWindow(U32 index) const;

    void handleWindowEvent(WindowEvent event, I64 winGUID, I32 data1, I32 data2);

    vec2<U16> getFullscreenResolution() const;

protected:
    U32 createAPIFlags(RenderAPI api);
    void pollSDLEvents();
protected:
    U32 _apiFlags;
    I32 _displayIndex;
    I64 _activeWindowGUID;
    I64 _mainWindowGUID;
    I64 _focusedWindowGUID;
    PlatformContext* _context;
    vectorImpl<DisplayWindow*> _windows;
};
}; //namespace Divide
#endif //_CORE_WINDOW_MANAGER_H_

#include "WindowManager.inl"