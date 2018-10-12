#include "stdafx.h"

#include "Headers/DisplayWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"

#ifndef HAVE_M_PI
#define HAVE_M_PI
#endif //HAVE_M_PI

#include <SDL.h>

namespace Divide {

namespace {
    SDL_SystemCursor CursorToSDL(CursorStyle style) {
        switch (style) {
            case CursorStyle::ARROW: return SDL_SYSTEM_CURSOR_ARROW;
            case CursorStyle::HAND: return SDL_SYSTEM_CURSOR_HAND;
            case CursorStyle::NONE: return SDL_SYSTEM_CURSOR_NO;
            case CursorStyle::RESIZE_EW: return SDL_SYSTEM_CURSOR_SIZEWE;
            case CursorStyle::RESIZE_NS: return SDL_SYSTEM_CURSOR_SIZENS;
            case CursorStyle::RESIZE_NESW: return SDL_SYSTEM_CURSOR_SIZENESW;
            case CursorStyle::RESIZE_NWSE: return SDL_SYSTEM_CURSOR_SIZENWSE;
            case CursorStyle::TEXT_INPUT: return SDL_SYSTEM_CURSOR_IBEAM;
        };

        return SDL_SYSTEM_CURSOR_NO;
    }
}; // namespace 

DisplayWindow::DisplayWindow(WindowManager& parent, PlatformContext& context)
 : GUIDWrapper(),
   PlatformContextComponent(context),
   _parent(parent),
   _swapBuffers(true),
   _hasFocus(true),
   _minimized(false),
   _maximized(false),
   _hidden(true),
   _warp(false),
   _vsync(false),
   _warpRect(-1),
   _opacity(255),
   _type(WindowType::COUNT),
   _previousType(WindowType::COUNT),
   _queuedType(WindowType::COUNT),
   _sdlWindow(nullptr),
   _userData(nullptr),
   _internalMoveEvent(false),
   _externalResizeEvent(false),
   _clearColour(DefaultColours::DIVIDE_BLUE),
   _windowID(std::numeric_limits<Uint32>::max()),
   _inputHandler(std::make_unique<Input::InputInterface>(*this))
{
    _prevDimensions.set(1);
    _windowDimensions.set(1);

    for (U8 i = 0; i < to_U8(CursorStyle::COUNT); ++i) {
        CursorStyle style = static_cast<CursorStyle>(i);
        _cursors[style] = SDL_CreateSystemCursor(CursorToSDL(style));
    }
}

DisplayWindow::~DisplayWindow() 
{
    for (auto it : _cursors) {
        SDL_FreeCursor(it.second);
    }
    _cursors.clear();

    destroyWindow();
}

ErrorCode DisplayWindow::destroyWindow() {

    if (_type != WindowType::COUNT && _sdlWindow != nullptr) {
        if (_destroyCbk) {
            _destroyCbk();
        }

        _inputHandler->terminate();
        _parent.destroyAPISettings(this);
        SDL_DestroyWindow(_sdlWindow);
        _sdlWindow = nullptr;
    }

    return ErrorCode::NO_ERR;
}

ErrorCode DisplayWindow::init(U32 windowFlags,
                              WindowType initialType,
                              const WindowDescriptor& descriptor)
{
    _vsync = BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::VSYNC));
    _type = initialType;
    _title = descriptor.title;
    _windowDimensions = descriptor.dimensions;
    const vec2<I16>& position = descriptor.position;

    _sdlWindow = SDL_CreateWindow(_title.c_str(),
                                  position.x == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(descriptor.targetDisplay) : position.x,
                                  position.y == -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(descriptor.targetDisplay) : position.y,
                                  1,
                                  1,
                                  windowFlags);

    _windowID = SDL_GetWindowID(_sdlWindow);
    
    I32 positionX, positionY;
    SDL_GetWindowPosition(_sdlWindow, &positionX, &positionY);
    setPosition(positionX, positionY);

    getWindowHandle(_sdlWindow, _handle);

    // Check if we have a valid window
    if (!_sdlWindow) {
        SDL_Quit();
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE")),
                         Locale::get(_ID("ERROR_SDL_WINDOW")));
        Console::printfn(Locale::get(_ID("WARN_APPLICATION_CLOSE")));
        return ErrorCode::SDL_WINDOW_INIT_ERROR;
    }

    return _inputHandler->init(_windowDimensions);
}

void DisplayWindow::update(const U64 deltaTimeUS) {
    if (hasFocus()) {
        _inputHandler->update(deltaTimeUS);
    }
    if (_queuedType != WindowType::COUNT) {
        //handleChangeWindowType(_queuedType);
        _queuedType = WindowType::COUNT;
    }
}

void DisplayWindow::notifyListeners(WindowEvent event, const WindowEventArgs& args) {
    for (auto listener : _eventListeners[to_base(event)]) {
        listener->_callback(args);
    }
}

void DisplayWindow::handleEvent(SDL_Event event) {
    if (event.type != SDL_WINDOWEVENT) {
        return;
    }
    if (event.window.windowID != _windowID) {
        return;
    }

    WindowEventArgs args;
    args._windowGUID = getGUID();

    switch (event.window.event) {
        case SDL_WINDOWEVENT_CLOSE: {
            notifyListeners(WindowEvent::CLOSE_REQUESTED, args);
            _parent.handleWindowEvent(WindowEvent::CLOSE_REQUESTED,
                                        getGUID(),
                                        event.quit.type,
                                        event.quit.timestamp);
        } break;
        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_FOCUS_GAINED: {
            args._flag = true;
            notifyListeners(WindowEvent::GAINED_FOCUS, args);
            _parent.handleWindowEvent(WindowEvent::GAINED_FOCUS,
                                      getGUID(),
                                      event.window.data1,
                                      event.window.data2);
        } break;
        case SDL_WINDOWEVENT_LEAVE:
        case SDL_WINDOWEVENT_FOCUS_LOST: {
            args._flag = false;
            notifyListeners(WindowEvent::LOST_FOCUS, args);

            _parent.handleWindowEvent(WindowEvent::LOST_FOCUS,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED: {
            _externalResizeEvent = event.window.event == SDL_WINDOWEVENT_RESIZED;
            args.x = event.window.data1;
            args.y = event.window.data2;
            args._flag = _externalResizeEvent;
            notifyListeners(WindowEvent::RESIZED, args);

            _parent.handleWindowEvent(WindowEvent::RESIZED,
                                        getGUID(),
                                        event.window.data1,
                                        event.window.data2,
                                        _externalResizeEvent);
            _externalResizeEvent = false;
        } break;
        case SDL_WINDOWEVENT_MOVED: {
            args.x = event.window.data1;
            args.y = event.window.data2;
            notifyListeners(WindowEvent::MOVED, args);

            _parent.handleWindowEvent(WindowEvent::MOVED,
                getGUID(),
                event.window.data1,
                event.window.data2);

            if (!_internalMoveEvent) {
                setPosition(event.window.data1,
                            event.window.data2);
                _internalMoveEvent = false;
            }
        } break;
        case SDL_WINDOWEVENT_SHOWN: {
            notifyListeners(WindowEvent::SHOWN, args);
                        
            _parent.handleWindowEvent(WindowEvent::SHOWN,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
        case SDL_WINDOWEVENT_HIDDEN: {
            notifyListeners(WindowEvent::HIDDEN, args);

            _parent.handleWindowEvent(WindowEvent::HIDDEN,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
        case SDL_WINDOWEVENT_MINIMIZED: {
            notifyListeners(WindowEvent::MINIMIZED, args);

            _parent.handleWindowEvent(WindowEvent::MINIMIZED,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
        case SDL_WINDOWEVENT_MAXIMIZED: {
            notifyListeners(WindowEvent::MAXIMIZED, args);

            _parent.handleWindowEvent(WindowEvent::MAXIMIZED,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
        case SDL_WINDOWEVENT_RESTORED: {
            notifyListeners(WindowEvent::RESTORED, args);

            _parent.handleWindowEvent(WindowEvent::RESTORED,
                getGUID(),
                event.window.data1,
                event.window.data2);
        } break;
    };
}

bool DisplayWindow::setDimensionsInternal(U16& w, U16& h) {
    if (_externalResizeEvent && 
        (_type != WindowType::WINDOW &&
         _type != WindowType::SPLASH))
    {
        return false;
    }

    I32 newW = to_I32(w); I32 newH = to_I32(h);
    if (_type == WindowType::FULLSCREEN) {
        // Find a decent resolution close to our dragged dimensions
        SDL_DisplayMode mode, closestMode;
        SDL_GetCurrentDisplayMode(currentDisplayIndex(), &mode);
        mode.w = w;
        mode.h = h;
        SDL_GetClosestDisplayMode(currentDisplayIndex(), &mode, &closestMode);
        w = to_U16(closestMode.w);
        h = to_U16(closestMode.h);
        SDL_SetWindowDisplayMode(_sdlWindow, &closestMode);
    } else {
        SDL_SetWindowSize(_sdlWindow, to_I32(w), to_I32(h));
        SDL_GetWindowSize(_sdlWindow, &newW, &newH);
    }

    if (_inputHandler->isInit()) {
        _inputHandler->onChangeWindowSize(w, h);
    }

    return newW == w && newH == h;
}

I32 DisplayWindow::currentDisplayIndex() const {
    I32 displayIndex = SDL_GetWindowDisplayIndex(_sdlWindow);
    assert(displayIndex != -1);
    return displayIndex;
}

vec2<U16> DisplayWindow::getDrawableSize() const {
    return context().gfx().getDrawableSize(*this);
}

void DisplayWindow::opacity(U8 opacity) {
    if (SDL_SetWindowOpacity(_sdlWindow, opacity / 255.0f) != -1) {
        _opacity = opacity;
    }
}

/// Window positioning is handled by SDL
void DisplayWindow::setPosition(I32 w, I32 h, bool global) {
    _internalMoveEvent = true;
    I32 displayIndex = currentDisplayIndex();

    if (!global) {

        if (w != -1) {
            w += _parent.monitorData()[displayIndex].viewport.x;
        }
        if (h != -1) {
            h += _parent.monitorData()[displayIndex].viewport.y;
        }
    }

    SDL_SetWindowPosition(_sdlWindow, 
                          w = -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex) : w,
                          h = -1 ? SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex) : h);
}

vec2<I32> DisplayWindow::getPosition(bool global) const {
    vec2<I32> ret;
    SDL_GetWindowPosition(_sdlWindow, &ret.x, &ret.y);

    if (!global) {
        vec2<I32> offset = _parent.monitorData()[currentDisplayIndex()].viewport.xy();
        ret -= offset;
    }

    return ret;
}

void DisplayWindow::hasFocus(const bool state) {
    _hasFocus = state;
    if (state) {
        SDL_RaiseWindow(_sdlWindow);
    }
}

/// Centering is also easier via SDL
void DisplayWindow::centerWindowPosition() {
    _internalMoveEvent = true;

    setPosition(-1, -1);
}

/// Mouse positioning is handled by SDL
void DisplayWindow::setCursorPosition(I32 x, I32 y) {
    SDL_WarpMouseInWindow(_sdlWindow, x, y);
}

void DisplayWindow::setCursorStyle(CursorStyle style) {
    SDL_SetCursor(_cursors[style]);
}

void DisplayWindow::hidden(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_SHOWN)) == state)
    {
        if (state) {
            SDL_HideWindow(_sdlWindow);
        } else {
            SDL_ShowWindow(_sdlWindow);
        }
        _hidden = state;
    }
}

void DisplayWindow::restore() {
    SDL_RestoreWindow(_sdlWindow);
    _maximized = _minimized = false;
}

void DisplayWindow::minimized(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_MINIMIZED)) != state)
    {
        if (state) {
            SDL_MinimizeWindow(_sdlWindow);
        } else {
            restore();
        }

        _minimized = state;
    }
}

void DisplayWindow::maximized(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_MAXIMIZED)) != state)
    {
        if (state) {
            SDL_MaximizeWindow(_sdlWindow);
        } else {
            restore();
        }
        _maximized = state;
    }
}

void DisplayWindow::title(const stringImpl& title) {
    if (title != _title) {
        SDL_SetWindowTitle(_sdlWindow, title.c_str());
        _title = title;
    }
}

void DisplayWindow::handleChangeWindowType(WindowType newWindowType) {
    _previousType = _type;
    _type = newWindowType;
    I32 switchState = 0;
    switch (newWindowType) {
        case WindowType::SPLASH: {
            switchState = SDL_SetWindowFullscreen(_sdlWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_sdlWindow, SDL_FALSE);
            SDL_SetWindowGrab(_sdlWindow, SDL_FALSE);
        } break;
        case WindowType::WINDOW: {
            switchState = SDL_SetWindowFullscreen(_sdlWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_sdlWindow, SDL_TRUE);
            SDL_SetWindowGrab(_sdlWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN_WINDOWED: {
            switchState = SDL_SetWindowFullscreen(_sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_sdlWindow, SDL_FALSE);
            SDL_SetWindowGrab(_sdlWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN: {
            switchState = SDL_SetWindowFullscreen(_sdlWindow, SDL_WINDOW_FULLSCREEN);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_sdlWindow, SDL_FALSE);
            SDL_SetWindowGrab(_sdlWindow, SDL_TRUE);
        } break;
    };

    centerWindowPosition();

    _parent.pollSDLEvents();
}

vec2<U16> DisplayWindow::getPreviousDimensions() const {
    if (fullscreen()) {
        return _parent.getFullscreenResolution();
    }
    return _prevDimensions;
}

bool DisplayWindow::setDimensions(U16& dimensionX, U16& dimensionY) {
    if (fullscreen()) {
        return true;
    }

    vec2<U16> newDimensions(dimensionX, dimensionY);
    if (_windowDimensions == newDimensions) {
        return true;
    }

    if (setDimensionsInternal(dimensionX, dimensionY)) {

        _prevDimensions.set(_windowDimensions);
        _windowDimensions.set(dimensionX, dimensionY);
        _parent.pollSDLEvents();
        return true;
    }

    return false;
}

bool DisplayWindow::setDimensions(vec2<U16>& dimensions) {
    return setDimensions(dimensions.x, dimensions.y);
}

vec2<U16> DisplayWindow::getDimensions() const {
    if (fullscreen()) {
        return _parent.getFullscreenResolution();
    }

    return _windowDimensions;
}

/// Key pressed: return true if input was consumed
bool DisplayWindow::onKeyDown(const Input::KeyEvent& key) {
    DisplayWindow::WindowEventArgs args;
    args._windowGUID = getGUID();
    args._key = key._key;
    args._flag = true;
    notifyListeners(WindowEvent::KEY_PRESS, args);

    return _context.app().kernel().onKeyDown(key);
}

/// Key released: return true if input was consumed
bool DisplayWindow::onKeyUp(const Input::KeyEvent& key) {
    DisplayWindow::WindowEventArgs args;
    args._windowGUID = getGUID();
    args._key = key._key;
    args._flag = false;
    notifyListeners(WindowEvent::KEY_PRESS, args);

    return _context.app().kernel().onKeyUp(key);
}

/// Joystick axis change: return true if input was consumed
bool DisplayWindow::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return _context.app().kernel().joystickAxisMoved(arg, axis);
}

/// Joystick direction change: return true if input was consumed
bool DisplayWindow::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    return _context.app().kernel().joystickPovMoved(arg, pov);
}

/// Joystick button pressed: return true if input was consumed
bool DisplayWindow::joystickButtonPressed(const Input::JoystickEvent& arg, Input::JoystickButton button) {
    return _context.app().kernel().joystickButtonPressed(arg, button);
}
/// Joystick button released: return true if input was consumed
bool DisplayWindow::joystickButtonReleased(const Input::JoystickEvent& arg, Input::JoystickButton button) {
    return _context.app().kernel().joystickButtonReleased(arg, button);
}
bool DisplayWindow::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    return _context.app().kernel().joystickSliderMoved(arg, index);
}

bool DisplayWindow::joystickvector3Moved(const Input::JoystickEvent& arg, I8 index) {
    return _context.app().kernel().joystickvector3Moved(arg, index);
}
/// Mouse moved: return true if input was consumed
bool DisplayWindow::mouseMoved(const Input::MouseEvent& arg) {
    DisplayWindow::WindowEventArgs args;
    args._windowGUID = getGUID();

    if (arg.Z().rel != 0) {
        args._mod = arg.Z().rel / 60;
        notifyListeners(WindowEvent::MOUSE_WHEEL, args);
    } else {
        args.x = arg.X().abs;
        args.y = arg.Y().abs;
        notifyListeners(WindowEvent::MOUSE_MOVE, args);
    }

    return _context.app().kernel().mouseMoved(arg);
}

/// Mouse button pressed: return true if input was consumed
bool DisplayWindow::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    DisplayWindow::WindowEventArgs args;
    args._windowGUID = getGUID();
    args.id = to_I32(button);
    args._flag = true;
    notifyListeners(WindowEvent::MOUSE_BUTTON, args);

    return _context.app().kernel().mouseButtonPressed(arg, button);
}

/// Mouse button released: return true if input was consumed
bool DisplayWindow::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    DisplayWindow::WindowEventArgs args;
    args._windowGUID = getGUID();
    args.id = to_I32(button);
    args._flag = false;
    notifyListeners(WindowEvent::MOUSE_BUTTON, args);

    return _context.app().kernel().mouseButtonReleased(arg, button);
}

void DisplayWindow::warp(bool state, const Rect<I32>& rect) {
    _warp = state;
    if (_warp) {
        _warpRect.set(rect);
    }
}

void DisplayWindow::renderingViewport(const Rect<I32>& viewport) {
    _renderingViewport.set(viewport);
}

}; //namespace Divide