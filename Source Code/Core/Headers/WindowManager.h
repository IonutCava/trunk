/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _CORE_WINDOW_MANAGER_H_
#define _CORE_WINDOW_MANAGER_H_

#include "Platform/Headers/DisplayWindow.h"
#include "Utility/Headers/Colours.h"

namespace Divide {

enum class RenderAPI : U8;

struct WindowDescriptor {
    enum class Flags : U16 {
        FULLSCREEN = toBit(1),
        DECORATED = toBit(2),
        RESIZEABLE = toBit(3),
        HIDDEN = toBit(4),
        ALLOW_HIGH_DPI = toBit(5),
        ALWAYS_ON_TOP = toBit(6),
        CLEAR_DEPTH = toBit(7),
        CLEAR_COLOUR = toBit(8),
        VSYNC = toBit(9),
        SHARE_CONTEXT = toBit(10)
    };

    U32 targetDisplay = 0u;
    stringImpl title = "";
    vec2<I16> position;
    vec2<U16> dimensions;
    FColour clearColour = DefaultColours::DIVIDE_BLUE;
    U32 flags = to_U32(to_base(Flags::DECORATED) |
                to_base(Flags::RESIZEABLE) |
                to_base(Flags::ALLOW_HIGH_DPI) |
                to_base(Flags::CLEAR_DEPTH) | 
                to_base(Flags::CLEAR_COLOUR));
};

class PlatformContext;
class WindowManager {
public:
    struct MonitorData {
        Rect<U16> viewport;
        Rect<U16> drawableArea;
        float dpi = 0.f;
    };

public:
    WindowManager() noexcept;
    ~WindowManager();

    ErrorCode init(PlatformContext& context,
                   const vec2<I16>& initialPosition,
                   const vec2<U16>& initialResolution,
                   bool startFullScreen,
                   I32 targetDisplayIndex);

    void close();

    void update(const U64 deltaTimeUS);

    bool anyWindowFocus() const;

    void setActiveWindow(U32 index);
    U32 createWindow(const WindowDescriptor& descriptor, ErrorCode& err);
    bool destroyWindow(DisplayWindow*& window);

    inline I32 targetDisplay() const;
    inline void targetDisplay(I32 displayIndex);

    void setCursorPosition(I32 x, I32 y, bool global = false);
    void snapCursorToCenter();

    static vec2<I32> getCursorPosition(bool global);
    static U32 getMouseState(vec2<I32>& pos, bool global);
    static void setCaptureMouse(bool state);

    inline DisplayWindow& getMainWindow();
    inline const DisplayWindow& getMainWindow() const;

    inline DisplayWindow& getActiveWindow();
    inline const DisplayWindow& getActiveWindow() const;

    inline DisplayWindow& getWindow(I64 guid);
    inline const DisplayWindow& getWindow(I64 guid) const;

    inline DisplayWindow& getWindow(U32 index);
    inline const DisplayWindow& getWindow(U32 index) const;

    inline U32 getWindowCount() const;

    inline const vector<MonitorData>& monitorData() const;

    void handleWindowEvent(WindowEvent event, I64 winGUID, I32 data1, I32 data2, bool flag = false);

    vec2<U16> getFullscreenResolution() const;

    void captureMouse(bool state);

    static void setCursorStyle(CursorStyle style);

protected:
    friend class DisplayWindow;
    void pollSDLEvents();

protected:
    U32 createAPIFlags(RenderAPI api);
    ErrorCode configureAPISettings(DisplayWindow* window, U32 descriptorFlags);
    void destroyAPISettings(DisplayWindow* window);

protected:
    U32 _apiFlags;
    I64 _activeWindowGUID;
    I64 _mainWindowGUID;
    I64 _focusedWindowGUID;

    vector<MonitorData> _monitors;
    PlatformContext* _context;
    vector<DisplayWindow*> _windows;

    static hashMap<CursorStyle, SDL_Cursor*> s_cursors;
};
}; //namespace Divide
#endif //_CORE_WINDOW_MANAGER_H_

#include "WindowManager.inl"