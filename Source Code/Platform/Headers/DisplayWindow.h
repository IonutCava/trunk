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

#ifndef _DISPLAY_WINDOW_H_
#define _DISPLAY_WINDOW_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

typedef struct SDL_Window SDL_Window;

namespace Divide {

enum class WindowType : U32 {
    WINDOW = 0,
    SPLASH = 1,
    FULLSCREEN = 2,
    FULLSCREEN_WINDOWED = 3,
    COUNT
};

enum class CursorStyle : U32 {
    NONE = 0,
    ARROW,
    TEXT_INPUT,
    HAND,
    RESIZE_NS,
    RESIZE_EW,
    RESIZE_NESW,
    RESIZE_NWSE,
    COUNT
};

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
    APP_LOOP = 11,
    CLOSE_REQUESTED = 12,
    KEY_PRESS = 13,
    MOUSE_MOVE = 14,
    MOUSE_BUTTON = 15,
    MOUSE_WHEEL = 16,
    CHAR = 17,
    COUNT
};
typedef std::array<vec2<I32>, to_base(WindowType::COUNT)> PositionByType;
typedef std::array<vec2<U16>, to_base(WindowType::COUNT)> ResolutionByType;

class WindowManager;
class PlatformContext;
enum class ErrorCode;
// Platform specific window
class DisplayWindow : public GUIDWrapper {
public:
    struct WindowEventArgs {
        bool _flag = false;
        Input::KeyCode _key;
        char _char = ' ';
        I32 x = -1, y = -1;
        I32 id = -1;

    };
    typedef DELEGATE_CBK<void, const WindowEventArgs&> EventListener;

public:
    DisplayWindow(WindowManager& parent, PlatformContext& context);
    ~DisplayWindow();
    ErrorCode init(U32 windowFlags,
                   WindowType initialType,
                   const ResolutionByType& initialResolutions,
                   const char* windowTitle);
    void update();

    ErrorCode destroyWindow();

    inline SDL_Window* getRawWindow() const;

    inline bool swapBuffers() const;
    inline void swapBuffers(const bool state);

    inline bool hasFocus() const;
    inline void hasFocus(const bool state);

    inline bool minimized() const;
           void minimized(const bool state);

    inline bool maximized() const;
           void maximized(const bool state);

    inline bool hidden() const;
           void hidden(const bool state);

    inline WindowType type() const;
    inline void type(WindowType type);
    inline void previousType();

    inline void setDimensions(U16 dimensionX, U16 dimensionY);
    inline void setDimensions(WindowType windowType, U16 dimensionX, U16 dimensionY);
    inline void setDimensions(WindowType windowType, const vec2<U16>& dimensions);

    inline const vec2<U16>& getDimensions(WindowType windowType) const;
    inline const vec2<U16>& getPreviousDimensions(WindowType windowType) const;

    inline const vec2<U16>& getDimensions() const;
    inline const vec2<U16>& getPreviousDimensions() const;

    inline void setPosition(I32 positionX, I32 positionY);
    inline void setPosition(WindowType windowType, I32 positionX, I32 positionY);
    inline void setPosition(WindowType windowType, const vec2<I32>& position);
    inline const vec2<I32>& getPosition(WindowType windowType) const;
    inline const vec2<I32>& getPosition() const;

    inline const stringImpl& title() const;
                        void title(const stringImpl& title);

    inline const WindowHandle& handle() const;

    /// Mouse positioning is handled by SDL
    void setCursorPosition(I32 x, I32 y) const;
    void setCursorStyle(CursorStyle style) const;

    inline void addEventListener(WindowEvent windowEvent, const EventListener& listener);

private:
    /// Internally change window size
    void setDimensionsInternal(U16 w, U16 h);
    /// Window positioning is handled by SDL
    void setPositionInternal(I32 w, I32 h);
    /// Centering is also easier via SDL
    void centerWindowPosition();
    /// Changing from one window type to another
    /// should also change display dimensions and position
    void handleChangeWindowType(WindowType newWindowType);

private:
    WindowManager& _parent;
    PlatformContext& _context;
    SDL_Window* _sdlWindow;

    /// The current rendering window type
    WindowType _type;
    WindowType _previousType;
    WindowType _queuedType;
    bool _swapBuffers;
    /// this is false if the window/application lost focus (e.g. clicked another
    /// window, alt + tab, etc)
    bool _hasFocus;
    bool _minimized;
    bool _maximized;
    bool _hidden;
    stringImpl _title;
    /// Did we generate the window move event?
    bool _internalMoveEvent;
    /// Did we resize the window via an OS call?
    bool _externalResizeEvent;

    PositionByType   _windowPosition;
    ResolutionByType _prevDimensions;
    ResolutionByType _windowDimensions;

    typedef vectorImpl<EventListener> EventListeners;
    std::array<EventListeners, to_base(WindowEvent::COUNT)> _eventListeners;

    // Varies from OS to OS
    WindowHandle _handle;

}; //DisplayWindow

}; //namespace Divide

#include "DisplayWindow.inl"

#endif //_DISPLAY_WINDOW_H_
