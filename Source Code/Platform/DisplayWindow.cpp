#include "Headers/DisplayWindow.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"

#ifndef HAVE_M_PI
#define HAVE_M_PI
#endif //HAVE_M_PI

#include <SDL.h>

namespace Divide {
DisplayWindow::DisplayWindow(WindowManager& context)
 : GUIDWrapper(),
   _context(context),
   _hasFocus(true),
   _minimized(false),
   _hidden(true),
   _type(WindowType::COUNT),
   _previousType(WindowType::COUNT),
   _queuedType(WindowType::COUNT),
   _mainWindow(nullptr),
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
        DIVIDE_ASSERT(_mainWindow != nullptr,
            "DisplayWindow::destroyWindow error: Tried to double-delete the same window!");

        SDL_DestroyWindow(_mainWindow);
        _mainWindow = nullptr;
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

    _mainWindow = SDL_CreateWindow(windowTitle,
                                   SDL_WINDOWPOS_CENTERED_DISPLAY(_context.targetDisplay()),
                                   SDL_WINDOWPOS_CENTERED_DISPLAY(_context.targetDisplay()),
                                   1,
                                   1,
                                   windowFlags);

    I32 positionX, positionY;
    SDL_GetWindowPosition(_mainWindow, &positionX, &positionY);
    setPosition(type(), positionX, positionY);

    // Check if we have a valid window
    if (!_mainWindow) {
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
        switch (event.type)
        {
        case SDL_QUIT: {
            SDL_HideWindow(_mainWindow);
            Application::instance().RequestShutdown();
        } break;
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_ENTER:
            case SDL_WINDOWEVENT_FOCUS_GAINED: {
                _context.handleWindowEvent(WindowEvent::GAINED_FOCUS,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_LEAVE:
            case SDL_WINDOWEVENT_FOCUS_LOST: {
                _context.handleWindowEvent(WindowEvent::LOST_FOCUS,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_RESIZED: {
                _externalResizeEvent = true;
                setDimensions(type(),
                              to_ushort(event.window.data1),
                              to_ushort(event.window.data2));
                _externalResizeEvent = false;
            }break;
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                _context.handleWindowEvent(WindowEvent::RESIZED_INTERNAL,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MOVED: {
                _context.handleWindowEvent(WindowEvent::MOVED,
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
                _context.handleWindowEvent(WindowEvent::SHOWN,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_HIDDEN: {
                _context.handleWindowEvent(WindowEvent::HIDDEN,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MINIMIZED: {
                _context.handleWindowEvent(WindowEvent::MAXIMIZED,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MAXIMIZED: {
                _context.handleWindowEvent(WindowEvent::MAXIMIZED,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_RESTORED: {
                _context.handleWindowEvent(WindowEvent::RESTORED,
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
        SDL_GetCurrentDisplayMode(_context.targetDisplay(), &mode);
        mode.w = w;
        mode.h = h;
        SDL_GetClosestDisplayMode(_context.targetDisplay(), &mode, &closestMode);
        SDL_SetWindowDisplayMode(_mainWindow, &closestMode);
    } else {
        if (_externalResizeEvent) {
            // Find a decent resolution close to our dragged dimensions
            SDL_DisplayMode mode, closestMode;
            SDL_GetCurrentDisplayMode(_context.targetDisplay(), &mode);
            mode.w = w;
            mode.h = h;
            SDL_GetClosestDisplayMode(_context.targetDisplay(), &mode, &closestMode);
            w = to_ushort(closestMode.w);
            h = to_ushort(closestMode.h);
        }

        SDL_SetWindowSize(_mainWindow, w, h);
    }
}

/// Window positioning is handled by SDL
void DisplayWindow::setPositionInternal(I32 w, I32 h) {
    _internalMoveEvent = true;
    SDL_SetWindowPosition(_mainWindow, w, h);
}

/// Centering is also easier via SDL
void DisplayWindow::centerWindowPosition() {
    _internalMoveEvent = true;

    setPosition(type(),
                SDL_WINDOWPOS_CENTERED_DISPLAY(_context.targetDisplay()),
                SDL_WINDOWPOS_CENTERED_DISPLAY(_context.targetDisplay()));
}

/// Mouse positioning is handled by SDL
void DisplayWindow::setCursorPosition(I32 x, I32 y) const {
    SDL_WarpMouseInWindow(_mainWindow, x, y);
}

void DisplayWindow::handleChangeWindowType(WindowType newWindowType) {
    _previousType = _type;
    _type = newWindowType;
    I32 switchState = 0;
    switch (newWindowType) {
        case WindowType::SPLASH: {
            switchState = SDL_SetWindowFullscreen(_mainWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(_mainWindow, SDL_FALSE);
        } break;
        case WindowType::WINDOW: {
            switchState = SDL_SetWindowFullscreen(_mainWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_mainWindow, SDL_TRUE);
            SDL_SetWindowGrab(_mainWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN_WINDOWED: {
            switchState = SDL_SetWindowFullscreen(_mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(_mainWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN: {
            switchState = SDL_SetWindowFullscreen(_mainWindow, SDL_WINDOW_FULLSCREEN);
            assert(switchState >= 0);

            SDL_SetWindowBordered(_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(_mainWindow, SDL_TRUE);
        } break;
    };

    const vec2<U16>& dimensions = getDimensions(newWindowType);
    setDimensionsInternal(dimensions.width, dimensions.height);

    centerWindowPosition();

    if (hidden()) {
        SDL_ShowWindow(_mainWindow);
        hidden(false);
    }
}

}; //namespace Divide