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
#ifndef _DISPLAY_WINDOW_H_
#define _DISPLAY_WINDOW_H_

#include "Core/Headers/PlatformContextComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

typedef struct SDL_Window SDL_Window;

namespace Divide {

enum class WindowType : U8 {
    WINDOW = 0,
    SPLASH = 1,
    FULLSCREEN = 2,
    FULLSCREEN_WINDOWED = 3,
    COUNT
};

enum class CursorStyle : U8 {
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

enum class WindowEvent : U8 {
    HIDDEN = 0,
    SHOWN = 1,
    MINIMIZED = 2,
    MAXIMIZED = 3,
    RESTORED = 4,
    LOST_FOCUS = 5,
    GAINED_FOCUS = 6,
    RESIZED = 7,
    MOVED = 8,
    APP_LOOP = 9,
    CLOSE_REQUESTED = 10,
    KEY_PRESS = 11,
    MOUSE_MOVE = 12,
    MOUSE_BUTTON = 13,
    MOUSE_WHEEL = 14,
    TEXT = 15,
    APP_QUIT = 16,
    COUNT
};

namespace Input {
    class InputInterface;
};

class WindowManager;
class PlatformContext;

struct WindowDescriptor;

enum class ErrorCode : I8;
// Platform specific window
class DisplayWindow : public GUIDWrapper,
                      public PlatformContextComponent,
                      public Input::InputAggregatorInterface {
public:
    struct WindowEventArgs {
        I64 _windowGUID = -1;
        bool _flag = false;
        Input::KeyCode _key = Input::KeyCode::KC_0;
        const char* _text = nullptr;
        I32 _mod = 0;
        I32 x = -1, y = -1;
        I32 id = -1;
    };

    typedef DELEGATE_CBK<void, const WindowEventArgs&> EventListener;

protected:
    SET_SAFE_DELETE_FRIEND
    SET_DELETE_VECTOR_FRIEND

    friend class WindowManager;
    DisplayWindow(WindowManager& parent, PlatformContext& context);
    ~DisplayWindow();

public:

    ErrorCode init(U32 windowFlags,
                   WindowType initialType,
                   const WindowDescriptor& descriptor);
    void update(const U64 deltaTimeUS);

    ErrorCode destroyWindow();

    inline SDL_Window* getRawWindow() const;

    I32 currentDisplayIndex() const;

    inline bool swapBuffers() const;
    inline void swapBuffers(const bool state);

    inline bool hasFocus() const;
           void hasFocus(const bool state);

    inline bool minimized() const;
           void minimized(const bool state);

    inline bool maximized() const;
           void maximized(const bool state);

    inline bool hidden() const;
           void hidden(const bool state);

    inline bool fullscreen() const;

    inline WindowType type() const;
    inline void changeType(WindowType newType);
    inline void changeToPreviousType();

           void opacity(U8 opacity);
    inline U8   opacity() const;

    inline void clearColour(const FColour& colour);
    inline void clearColour(const FColour& colour, bool clearColour, bool clearDepth);

    inline const FColour& clearColour() const;
    inline const FColour& clearColour(bool &clearColour, bool &clearDepth) const;

    /// dimensionX and dimensionY get adjusted to the closest supported value
    bool setDimensions(U16& dimensionX, U16& dimensionY);
    bool setDimensions(vec2<U16>& dimensions);

    vec2<U16> getDimensions() const;
    vec2<U16> getPreviousDimensions() const;

    vec2<U16> getDrawableSize() const;
    vec2<I32> getPosition(bool global = false) const;

           void setPosition(I32 positionX, I32 positionY, bool global = false);
    inline void setPosition(const vec2<I32>& position, bool global = false);

    inline const stringImpl& title() const;
                        void title(const stringImpl& title);

    inline const WindowHandle& handle() const;

    /// Mouse positioning is handled by SDL
    void setCursorPosition(I32 x, I32 y);
    void setCursorStyle(CursorStyle style);

    inline I64 addEventListener(WindowEvent windowEvent, const EventListener& listener);
    inline void removeEventlistener(WindowEvent windowEvent, I64 listenerGUID);

    inline Input::InputInterface& inputHandler();

    void handleEvent(SDL_Event event);
    void notifyListeners(WindowEvent event, const WindowEventArgs& args);

    inline void destroyCbk(const DELEGATE_CBK<void>& destroyCbk);

    inline bool warp() const;
    inline const Rect<I32>& warpRect() const;
    void warp(bool state, const Rect<I32>& rect = Rect<I32>(-1));

    inline const Rect<I32>& renderingViewport() const;
    void renderingViewport(const Rect<I32>& viewport);

    inline void* userData() const;

private:
    void restore();
    /// Internally change window size
    bool setDimensionsInternal(U16& w, U16& h);
    /// Centering is also easier via SDL
    void centerWindowPosition();
    /// Changing from one window type to another
    /// should also change display dimensions and position
    void handleChangeWindowType(WindowType newWindowType);

protected: //Input
    /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystick axis change: return true if input was consumed
    bool joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis);
    /// Joystick direction change: return true if input was consumed
    bool joystickPovMoved(const Input::JoystickEvent& arg, I8 pov);
    /// Joystick button pressed: return true if input was consumed
    bool joystickButtonPressed(const Input::JoystickEvent& arg, Input::JoystickButton button);
    /// Joystick button released: return true if input was consumed
    bool joystickButtonReleased(const Input::JoystickEvent& arg, Input::JoystickButton button);
    bool joystickSliderMoved(const Input::JoystickEvent& arg, I8 index);
    bool joystickvector3Moved(const Input::JoystickEvent& arg, I8 index);
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button);
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button);

private:
    WindowManager& _parent;
    SDL_Window* _sdlWindow;

    /// The current rendering window type
    WindowType _type;
    WindowType _previousType;
    WindowType _queuedType;
    bool _swapBuffers;
    /// this is false if the window/application lost focus (e.g. clicked another window, alt + tab, etc)
    bool _hasFocus;
    bool _minimized;
    bool _maximized;
    bool _hidden;
    bool _vsync;
    stringImpl _title;
    /// Did we generate the window move event?
    bool _internalMoveEvent;
    /// Did we resize the window via an OS call?
    bool _externalResizeEvent;

    bool _warp;
    Rect<I32> _warpRect;
    Rect<I32> _renderingViewport;

    U8 _opacity;
    vec2<U16> _prevDimensions;
    vec2<U16> _windowDimensions;
    FColour   _clearColour;
    bool      _shouldClearColour;
    bool      _shouldClearDepth;
    typedef vector<std::shared_ptr<GUID_DELEGATE_CBK<void, WindowEventArgs>>> EventListeners;
    std::array<EventListeners, to_base(WindowEvent::COUNT)> _eventListeners;

    std::unique_ptr<Input::InputInterface> _inputHandler;
    Uint32 _windowID;

    DELEGATE_CBK<void> _destroyCbk;

    hashMap<CursorStyle, SDL_Cursor*> _cursors;
    void *_userData = nullptr;
    // Varies from OS to OS
    WindowHandle _handle;

}; //DisplayWindow

}; //namespace Divide

#include "DisplayWindow.inl"

#endif //_DISPLAY_WINDOW_H_
