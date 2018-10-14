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

DisplayWindow::DisplayWindow(WindowManager& parent, PlatformContext& context)
 : GUIDWrapper(),
   PlatformContextComponent(context),
   _parent(parent),
   _warpRect(-1),
   _opacity(255),
   _type(WindowType::COUNT),
   _previousType(WindowType::COUNT),
   _queuedType(WindowType::COUNT),
   _sdlWindow(nullptr),
   _userData(nullptr),
   _internalMoveEvent(false),
   _internalResizeEvent(false),
   _clearColour(DefaultColours::DIVIDE_BLUE),
   _windowID(std::numeric_limits<Uint32>::max()),
   _inputHandler(std::make_unique<Input::InputInterface>(*this))
{
    SetBit(_flags, WindowFlags::SWAP_BUFFER);

    _prevDimensions.set(1);
    _windowDimensions.set(1);
}

DisplayWindow::~DisplayWindow() 
{
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
    bool vsync = BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::VSYNC));
    ToggleBit(_flags, WindowFlags::VSYNC, vsync);

    _type = initialType;
    _title = descriptor.title;
    _windowDimensions = descriptor.dimensions;

    vec2<I32> position(descriptor.position);

    if (position.x == -1) {
        position.x = SDL_WINDOWPOS_CENTERED_DISPLAY(descriptor.targetDisplay);
    } else {
        position.x += _parent.monitorData()[descriptor.targetDisplay].viewport.x;
    }
    if (position.y == -1) {
        position.y = SDL_WINDOWPOS_CENTERED_DISPLAY(descriptor.targetDisplay);
    } else {
        position.y += _parent.monitorData()[descriptor.targetDisplay].viewport.x;
    }

    _sdlWindow = SDL_CreateWindow(_title.c_str(),
                                  position.x,
                                  position.y,
                                  _windowDimensions.width,
                                  _windowDimensions.height,
                                  windowFlags);

    _windowID = SDL_GetWindowID(_sdlWindow);
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
    assert(event.type == SDL_WINDOWEVENT);

    WindowEventArgs args = {};
    args._windowGUID = getGUID();
    args.x = event.window.data1;
    args.y = event.window.data2;

    switch (event.window.event) {
        case SDL_WINDOWEVENT_CLOSE: {
            args.x = event.quit.type;
            args.y = event.quit.timestamp;
            notifyListeners(WindowEvent::CLOSE_REQUESTED, args);
        } break;
        case SDL_WINDOWEVENT_ENTER: {
            ToggleBit(_flags, WindowFlags::IS_HOVERED, true);
            notifyListeners(WindowEvent::MOUSE_HOVER_ENTER, args);
        } break;
        case SDL_WINDOWEVENT_LEAVE: {
            ToggleBit(_flags, WindowFlags::IS_HOVERED, false);
            notifyListeners(WindowEvent::MOUSE_HOVER_LEAVE, args);
        } break;
        case SDL_WINDOWEVENT_FOCUS_GAINED: {
            ToggleBit(_flags, WindowFlags::HAS_FOCUS, true);
            notifyListeners(WindowEvent::GAINED_FOCUS, args);
        } break;
        case SDL_WINDOWEVENT_FOCUS_LOST: {
            ToggleBit(_flags, WindowFlags::HAS_FOCUS, false);
            notifyListeners(WindowEvent::LOST_FOCUS, args);
        } break;
        case SDL_WINDOWEVENT_RESIZED: {
            if (!_internalResizeEvent) {
                U16 width = to_U16(event.window.data1);
                U16 height = to_U16(event.window.data2);
                setDimensions(width, height);
            }
            _internalResizeEvent = false;
        }break;
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
            args._flag = fullscreen();
            notifyListeners(WindowEvent::RESIZED, args);
        } break;
        case SDL_WINDOWEVENT_MOVED: {
            notifyListeners(WindowEvent::MOVED, args);
            if (!_internalMoveEvent) {
                setPosition(event.window.data1,
                            event.window.data2);
                _internalMoveEvent = false;
            }
        } break;
        case SDL_WINDOWEVENT_SHOWN: {
            notifyListeners(WindowEvent::SHOWN, args);
            ToggleBit(_flags, WindowFlags::HIDDEN, false);
        } break;
        case SDL_WINDOWEVENT_HIDDEN: {
            notifyListeners(WindowEvent::HIDDEN, args);
            ToggleBit(_flags, WindowFlags::HIDDEN, true);
        } break;
        case SDL_WINDOWEVENT_MINIMIZED: {
            notifyListeners(WindowEvent::MINIMIZED, args);
            minimized(true);
        } break;
        case SDL_WINDOWEVENT_MAXIMIZED: {
            notifyListeners(WindowEvent::MAXIMIZED, args);
            minimized(false);
        } break;
        case SDL_WINDOWEVENT_RESTORED: {
            notifyListeners(WindowEvent::RESTORED, args);
            minimized(false);
        } break;
    };
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
void DisplayWindow::setPosition(I32 x, I32 y, bool global) {
    _internalMoveEvent = true;

    I32 displayIndex = currentDisplayIndex();
    if (x == -1) {
        x = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
    } else if (!global) {
        x += _parent.monitorData()[displayIndex].viewport.x;
    }
    if (y == -1) {
        y = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
    } else if (!global) {
        y += _parent.monitorData()[displayIndex].viewport.y;
    }

    SDL_SetWindowPosition(_sdlWindow, x, y);
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

void DisplayWindow::bringToFront() const {
    SDL_RaiseWindow(_sdlWindow);
}

/// Centering is also easier via SDL
void DisplayWindow::centerWindowPosition() {
    setPosition(-1, -1);
}

/// Mouse positioning is handled by SDL
void DisplayWindow::setCursorPosition(I32 x, I32 y) {
    SDL_WarpMouseInWindow(_sdlWindow, x, y);
}

void DisplayWindow::hidden(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_SHOWN)) == state) {
        if (state) {
            SDL_HideWindow(_sdlWindow);
        } else {
            SDL_ShowWindow(_sdlWindow);
        }
    }

    ToggleBit(_flags, WindowFlags::HIDDEN, state);
}

void DisplayWindow::restore() {
    SDL_RestoreWindow(_sdlWindow);

    ClearBit(_flags, WindowFlags::MAXIMIZED);
    ClearBit(_flags, WindowFlags::MINIMIZED);
}

void DisplayWindow::minimized(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_MINIMIZED)) != state) {
        if (state) {
            SDL_MinimizeWindow(_sdlWindow);
        } else {
            restore();
        }
    }

    ToggleBit(_flags, WindowFlags::MINIMIZED, state);
}

void DisplayWindow::maximized(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_MAXIMIZED)) != state) {
        if (state) {
            SDL_MaximizeWindow(_sdlWindow);
        } else {
            restore();
        }
    }

    ToggleBit(_flags, WindowFlags::MAXIMIZED, state);
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

bool DisplayWindow::setDimensions(U16& width, U16& height) {
    if (fullscreen() || _windowDimensions == vec2<U16>(width, height)) {
        return true;
    }

    _internalResizeEvent = true;

    I32 newW = to_I32(width);
    I32 newH = to_I32(height);
    if (_type == WindowType::FULLSCREEN) {
        // Find a decent resolution close to our dragged dimensions
        SDL_DisplayMode mode, closestMode;
        SDL_GetCurrentDisplayMode(currentDisplayIndex(), &mode);
        mode.w = width;
        mode.h = height;
        SDL_GetClosestDisplayMode(currentDisplayIndex(), &mode, &closestMode);
        width = to_U16(closestMode.w);
        height = to_U16(closestMode.h);
        SDL_SetWindowDisplayMode(_sdlWindow, &closestMode);
    } else {
        SDL_SetWindowSize(_sdlWindow, newW, newH);
        SDL_GetWindowSize(_sdlWindow, &newW, &newH);
    }

    if (newW == width && newH == height) {
        if (_inputHandler->isInit()) {
            _inputHandler->onChangeWindowSize(width, height);
        }

        _prevDimensions.set(_windowDimensions);
        _windowDimensions.set(width, height);
        _parent.pollSDLEvents();
        return true;
    }

    return false;
}

bool DisplayWindow::setDimensions(vec2<U16>& dimensions) {
    return setDimensions(dimensions.x, dimensions.y);
}

vec2<U16> DisplayWindow::getDimensions() const {
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
    ToggleBit(_flags, WindowFlags::WARP, state);
    _warpRect.set(rect);
}

void DisplayWindow::renderingViewport(const Rect<I32>& viewport) {
    _renderingViewport.set(viewport);
}

}; //namespace Divide