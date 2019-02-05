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

enum class WindowMode : U8 {
    WINDOWED = 0,
    BORDERLESS_WINDOWED,
    FULLSCREEN
};

struct WindowDescriptor {
    enum class Flags : U16 {
        FULLSCREEN = toBit(1),
        FULLSCREEN_DESKTOP = toBit(2),
        DECORATED = toBit(3),
        RESIZEABLE = toBit(4),
        HIDDEN = toBit(5),
        ALLOW_HIGH_DPI = toBit(6),
        ALWAYS_ON_TOP = toBit(7),
        CLEAR_DEPTH = toBit(8),
        CLEAR_COLOUR = toBit(9),
        VSYNC = toBit(10),
        SHARE_CONTEXT = toBit(11)
    };

    bool externalClose = false;
    U32 targetDisplay = 0u;
    stringImpl title = "";
    vec2<I16> position;
    vec2<U16> dimensions;
    FColour clearColour = DefaultColours::DIVIDE_BLUE;
    U32 flags = to_U32(Flags::DECORATED) |
                to_U32(Flags::RESIZEABLE) |
                to_U32(Flags::ALLOW_HIGH_DPI) |
                to_U32(Flags::CLEAR_DEPTH) |
                to_U32(Flags::CLEAR_COLOUR);
};

class PlatformContext;
class WindowManager : public SDLEventListener {
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
                   const vec2<U16>& initialSize,
                   WindowMode windowMode,
                   I32 targetDisplayIndex);

    void close();
    void hideAll();

    void update(const U64 deltaTimeUS);

    bool anyWindowFocus() const;

    inline DisplayWindow& createWindow(const WindowDescriptor& descriptor);
    inline DisplayWindow& createWindow(const WindowDescriptor& descriptor, ErrorCode& err);

    DisplayWindow& createWindow(const WindowDescriptor& descriptor, ErrorCode& err, U32& windowIndex);
    bool destroyWindow(DisplayWindow*& window);

    void setCursorPosition(I32 x, I32 y, bool global = false);
    void snapCursorToCenter();

    static vec2<I32> GetCursorPosition(bool global);
    static U32 GetMouseState(vec2<I32>& pos, bool global) noexcept;
    static void SetCaptureMouse(bool state) noexcept;

    inline DisplayWindow& getMainWindow();
    inline const DisplayWindow& getMainWindow() const;

    //Returns null if no window is currently focused
    inline DisplayWindow* getFocusedWindow();
    inline const DisplayWindow* getFocusedWindow() const;

    //Returns null if no window is currently hovered
    inline DisplayWindow* getHoveredWindow();
    inline const DisplayWindow* getHoveredWindow() const;

    inline DisplayWindow& getWindow(I64 guid);
    inline const DisplayWindow& getWindow(I64 guid) const;

    inline DisplayWindow& getWindow(U32 index);
    inline const DisplayWindow& getWindow(U32 index) const;

    inline DisplayWindow* getWindowByID(U32 ID) noexcept;
    inline const DisplayWindow* getWindowByID(U32 ID) const noexcept;

    inline U32 getWindowCount() const noexcept;

    inline const vector<MonitorData>& monitorData() const noexcept;

    vec2<U16> getFullscreenResolution() const;

    void captureMouse(bool state) noexcept;

    static void SetCursorStyle(CursorStyle style);

    static void ToggleRelativeMouseMode(bool state) noexcept;
protected:
    bool onSDLEvent(SDL_Event event) noexcept override;

protected:
    friend class DisplayWindow;
    U32 createAPIFlags(RenderAPI api) noexcept;
    ErrorCode configureAPISettings(U32 descriptorFlags);
    ErrorCode applyAPISettings(DisplayWindow* window, U32 descriptorFlags);
    void destroyAPISettings(DisplayWindow* window);

protected:
    U32 _apiFlags = 0;
    I64 _mainWindowGUID = -1;

    PlatformContext* _context = nullptr;
    vector<MonitorData> _monitors;
    vector<DisplayWindow*> _windows;

    static hashMap<CursorStyle, SDL_Cursor*> s_cursors;
};
}; //namespace Divide
#endif //_CORE_WINDOW_MANAGER_H_

#include "WindowManager.inl"