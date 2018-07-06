#include "stdafx.h"

#include "Headers/DisplayWindow.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"

#ifndef HAVE_M_PI
#define HAVE_M_PI
#endif //HAVE_M_PI

#include <SDL.h>

#include <AntTweakBar/include/AntTweakBar.h>

namespace Divide {

namespace {
    Input::KeyCode SDLToOIS(SDL_Keycode code) {
        switch (code) {
            case SDLK_TAB: return Input::KeyCode::KC_TAB;
            case SDLK_LEFT: return Input::KeyCode::KC_LEFT;
            case SDLK_RIGHT: return Input::KeyCode::KC_RIGHT;
            case SDLK_UP: return Input::KeyCode::KC_UP;
            case SDLK_DOWN: return Input::KeyCode::KC_DOWN;
            case SDLK_PAGEUP: return Input::KeyCode::KC_PGUP;
            case SDLK_PAGEDOWN: return Input::KeyCode::KC_PGDOWN;
            case SDLK_HOME: return Input::KeyCode::KC_HOME;
            case SDLK_END: return Input::KeyCode::KC_END;
            case SDLK_DELETE: return Input::KeyCode::KC_DELETE;
            case SDLK_BACKSPACE: return Input::KeyCode::KC_BACK;
            case SDLK_RETURN: return Input::KeyCode::KC_RETURN;
            case SDLK_ESCAPE : return Input::KeyCode::KC_ESCAPE;
            case SDLK_a: return Input::KeyCode::KC_A;
            case SDLK_c: return Input::KeyCode::KC_C;
            case SDLK_v: return Input::KeyCode::KC_V;
            case SDLK_x: return Input::KeyCode::KC_X;
            case SDLK_y: return Input::KeyCode::KC_Y;
            case SDLK_z: return Input::KeyCode::KC_Z;
        };
        return Input::KeyCode::KC_YEN;
    }

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
   _parent(parent),
   _context(context),
   _swapBuffers(true),
   _hasFocus(true),
   _minimized(false),
   _maximized(false),
   _hidden(true),
   _type(WindowType::COUNT),
   _previousType(WindowType::COUNT),
   _queuedType(WindowType::COUNT),
   _sdlWindow(nullptr),
   _internalMoveEvent(false),
   _externalResizeEvent(false)
{
    _windowPosition.fill(vec2<I32>(0));
    _prevDimensions.fill(vec2<U16>(1));
    _windowDimensions.fill(vec2<U16>(1));
}

DisplayWindow::~DisplayWindow() 
{
}

ErrorCode DisplayWindow::destroyWindow() {
    if (_type != WindowType::COUNT) {
        DIVIDE_ASSERT(_sdlWindow != nullptr,
            "DisplayWindow::destroyWindow error: Tried to double-delete the same window!");

        SDL_DestroyWindow(_sdlWindow);
        _sdlWindow = nullptr;
    }

    return ErrorCode::NO_ERR;
}

ErrorCode DisplayWindow::init(U32 windowFlags,
                              WindowType initialType,
                              const ResolutionByType& initialResolutions,
                              const char* windowTitle)
{
    _type = initialType;

    _windowDimensions = initialResolutions;

    _sdlWindow = SDL_CreateWindow(windowTitle,
                                  SDL_WINDOWPOS_CENTERED_DISPLAY(_parent.targetDisplay()),
                                  SDL_WINDOWPOS_CENTERED_DISPLAY(_parent.targetDisplay()),
                                  1,
                                  1,
                                  windowFlags);
    
    I32 positionX, positionY;
    SDL_GetWindowPosition(_sdlWindow, &positionX, &positionY);
    setPosition(type(), positionX, positionY);

    getWindowHandle(_sdlWindow, _handle);
    _title = windowTitle;

    // Check if we have a valid window
    if (!_sdlWindow) {
        SDL_Quit();
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE")),
                         Locale::get(_ID("ERROR_SDL_WINDOW")));
        Console::printfn(Locale::get(_ID("WARN_APPLICATION_CLOSE")));
        return ErrorCode::SDL_WINDOW_INIT_ERROR;
    }

    return ErrorCode::NO_ERR;
}

void DisplayWindow::update() {

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (Config::USE_ANT_TWEAK_BAR) {
            if (_context.config().gui.enableDebugVariableControls) {
                // send event to AntTweakBar
                if (TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION) != 0) {
                    continue;
                }
            }
        }
        WindowEventArgs args;

        switch (event.type)
        {
            case SDL_QUIT: {
                for (EventListener& listener : _eventListeners[to_base(WindowEvent::CLOSE_REQUESTED)]) {
                    listener(args);
                }

                _parent.handleWindowEvent(WindowEvent::CLOSE_REQUESTED,
                    getGUID(),
                    event.quit.type,
                    event.quit.timestamp);
            } break;

            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                if (hasFocus()) {
                    args._key = SDLToOIS(event.key.keysym.sym);
                    if (event.key.type == SDL_KEYUP) {
                        args._flag = false;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::KEY_PRESS)]) {
                            listener(args);
                        }
                    } else {
                        args._flag = true;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::KEY_PRESS)]) {
                            listener(args);
                        }
                    }
                }
            } break;
            case SDL_TEXTINPUT: {
                if (hasFocus()) {
                    args._char = event.text.text[0];
                    for (EventListener& listener : _eventListeners[to_base(WindowEvent::CHAR)]) {
                        listener(args);
                    }
                }
            } break;
            case SDL_MOUSEBUTTONDOWN: {
                if (hasFocus()) {
                    args._flag = true;
                    args.id = event.button.button;
                    for (EventListener& listener : _eventListeners[to_base(WindowEvent::MOUSE_BUTTON)]) {
                        listener(args);
                    }
                }
            } break;
            case SDL_MOUSEBUTTONUP: {
                if (hasFocus()) {
                    args._flag = false;
                    args.id = event.button.button;
                    for (EventListener& listener : _eventListeners[to_base(WindowEvent::MOUSE_BUTTON)]) {
                        listener(args);
                    }
                }
            } break;
            case SDL_MOUSEMOTION: {
                if (hasFocus()) {
                    args.x = event.motion.x;
                    args.y = event.motion.y;
                    for (EventListener& listener : _eventListeners[to_base(WindowEvent::MOUSE_MOVE)]) {
                        listener(args);
                    }
                }
            } break;
            case SDL_MOUSEWHEEL: {
                if (hasFocus()) {
                    args.x = event.wheel.x;
                    args.y = event.wheel.y;
                    for (EventListener& listener : _eventListeners[to_base(WindowEvent::MOUSE_WHEEL)]) {
                        listener(args);
                    }
                }
            } break;
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::CLOSE_REQUESTED)]) {
                            listener(args);
                        }
                    } break;
                    case SDL_WINDOWEVENT_ENTER:
                    case SDL_WINDOWEVENT_FOCUS_GAINED: {
                        args._flag = true;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::GAINED_FOCUS)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::GAINED_FOCUS,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_LEAVE:
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        args._flag = false;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::LOST_FOCUS)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::LOST_FOCUS,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_RESIZED: {
                        _externalResizeEvent = true;
                        args.x = event.window.data1;
                        args.y = event.window.data2;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::RESIZED_EXTERNAL)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::RESIZED_EXTERNAL,
                                                    getGUID(),
                                                    event.window.data1,
                                                    event.window.data2);
                        _externalResizeEvent = false;
                    } break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        args.x = event.window.data1;
                        args.y = event.window.data2;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::RESIZED_INTERNAL)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::RESIZED_INTERNAL,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MOVED: {
                        args.x = event.window.data1;
                        args.y = event.window.data2;
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::MOVED)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::MOVED,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);

                        if (!_internalMoveEvent) {
                            setPosition(type(),
                                        event.window.data1,
                                        event.window.data2);
                            _internalMoveEvent = false;
                        }
                    } break;
                    case SDL_WINDOWEVENT_SHOWN: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::SHOWN)]) {
                            listener(args);
                        }
                        
                        _parent.handleWindowEvent(WindowEvent::SHOWN,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_HIDDEN: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::HIDDEN)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::HIDDEN,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MINIMIZED: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::MINIMIZED)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::MINIMIZED,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MAXIMIZED: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::MAXIMIZED)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::MAXIMIZED,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_RESTORED: {
                        for (EventListener& listener : _eventListeners[to_base(WindowEvent::RESTORED)]) {
                            listener(args);
                        }

                        _parent.handleWindowEvent(WindowEvent::RESTORED,
                            getGUID(),
                            event.window.data1,
                            event.window.data2);
                    } break;
                };
            } break;
        }
    }

    if (_queuedType != WindowType::COUNT) {
        //handleChangeWindowType(_queuedType);
        _queuedType = WindowType::COUNT;
    }
}

void DisplayWindow::setDimensionsInternal(U16 w, U16 h) {
    if (_externalResizeEvent && 
        (_type != WindowType::WINDOW &&
         _type != WindowType::SPLASH))
    {
        return;
    }

    if (_type == WindowType::FULLSCREEN) {
        // Should never be true, but need to be sure
        SDL_DisplayMode mode, closestMode;
        SDL_GetCurrentDisplayMode(_parent.targetDisplay(), &mode);
        mode.w = w;
        mode.h = h;
        SDL_GetClosestDisplayMode(_parent.targetDisplay(), &mode, &closestMode);
        SDL_SetWindowDisplayMode(_sdlWindow, &closestMode);
    } else {
        if (_externalResizeEvent) {
            // Find a decent resolution close to our dragged dimensions
            SDL_DisplayMode mode, closestMode;
            SDL_GetCurrentDisplayMode(_parent.targetDisplay(), &mode);
            mode.w = w;
            mode.h = h;
            SDL_GetClosestDisplayMode(_parent.targetDisplay(), &mode, &closestMode);
            w = to_U16(closestMode.w);
            h = to_U16(closestMode.h);
        }

        SDL_SetWindowSize(_sdlWindow, w, h);
    }
}

/// Window positioning is handled by SDL
void DisplayWindow::setPositionInternal(I32 w, I32 h) {
    _internalMoveEvent = true;
    SDL_SetWindowPosition(_sdlWindow, w, h);
}

/// Centering is also easier via SDL
void DisplayWindow::centerWindowPosition() {
    _internalMoveEvent = true;

    setPosition(type(),
                SDL_WINDOWPOS_CENTERED_DISPLAY(_parent.targetDisplay()),
                SDL_WINDOWPOS_CENTERED_DISPLAY(_parent.targetDisplay()));
}

/// Mouse positioning is handled by SDL
void DisplayWindow::setCursorPosition(I32 x, I32 y) const {
    SDL_WarpMouseInWindow(_sdlWindow, x, y);
}

void DisplayWindow::setCursorStyle(CursorStyle style) const {
    SDL_SetCursor(SDL_CreateSystemCursor(CursorToSDL(style)));
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

void DisplayWindow::minimized(const bool state) {
    if (BitCompare(SDL_GetWindowFlags(_sdlWindow), to_U32(SDL_WINDOW_MINIMIZED)) != state)
    {
        if (state) {
            SDL_MinimizeWindow(_sdlWindow);
        } else {
            SDL_RestoreWindow(_sdlWindow);
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
            SDL_RestoreWindow(_sdlWindow);
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

    const vec2<U16>& dimensions = getDimensions(newWindowType);
    setDimensionsInternal(dimensions.width, dimensions.height);

    centerWindowPosition();

    if (hidden()) {
        hidden(false);
    }
}

}; //namespace Divide