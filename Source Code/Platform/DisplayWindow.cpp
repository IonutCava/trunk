#include "Headers/DisplayWindow.h"
#include "Core/Headers/Application.h"

#include "Core/Headers/ParamHandler.h"

#define HAVE_M_PI
#include <SDL.h>

namespace Divide {
DisplayWindow::DisplayWindow()
 : GUIDWrapper(),
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
}

DisplayWindow::~DisplayWindow() 
{
}

ErrorCode DisplayWindow::destroyWindow() {
    DIVIDE_ASSERT(_mainWindow != nullptr,
        "DisplayWindow::destroyWindow error: Tried to double-delete the same window!");

    SDL_DestroyWindow(_mainWindow);
    _mainWindow = nullptr;

    return ErrorCode::NO_ERR;
}

ErrorCode DisplayWindow::init(U32 windowFlags, WindowType initialType, ResolutionByType initialResolutions) {
    ParamHandler& par = ParamHandler::getInstance();
    Application& app = Application::getInstance();
    WindowManager& wManager = app.getWindowManager();

    _type = initialType;

    _windowDimensions = initialResolutions;

    _mainWindow = SDL_CreateWindow(par.getParam<stringImpl>("appTitle", "Divide").c_str(),
                                   SDL_WINDOWPOS_CENTERED_DISPLAY(wManager.targetDisplay()),
                                   SDL_WINDOWPOS_CENTERED_DISPLAY(wManager.targetDisplay()),
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
    Application& app = Application::getInstance();
    WindowManager& wManager = app.getWindowManager();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT: {
            SDL_HideWindow(_mainWindow);
            app.RequestShutdown();
        } break;
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_ENTER:
            case SDL_WINDOWEVENT_FOCUS_GAINED: {
                wManager.handleWindowEvent(WindowEvent::GAINED_FOCUS,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_LEAVE:
            case SDL_WINDOWEVENT_FOCUS_LOST: {
                wManager.handleWindowEvent(WindowEvent::LOST_FOCUS,
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
                wManager.handleWindowEvent(WindowEvent::RESIZED_INTERNAL,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MOVED: {
                wManager.handleWindowEvent(WindowEvent::MOVED,
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
                wManager.handleWindowEvent(WindowEvent::SHOWN,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_HIDDEN: {
                wManager.handleWindowEvent(WindowEvent::HIDDEN,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MINIMIZED: {
                wManager.handleWindowEvent(WindowEvent::MAXIMIZED,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_MAXIMIZED: {
                wManager.handleWindowEvent(WindowEvent::MAXIMIZED,
                    getGUID(),
                    event.window.data1,
                    event.window.data2);
            } break;
            case SDL_WINDOWEVENT_RESTORED: {
                wManager.handleWindowEvent(WindowEvent::RESTORED,
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
    WindowManager& winManager = Application::getInstance().getWindowManager();

    if (_externalResizeEvent && 
        (_type != WindowType::WINDOW &&
         _type != WindowType::SPLASH))
    {
        return;
    }

    if (_type == WindowType::FULLSCREEN) {
        // Should never be true, but need to be sure
        SDL_DisplayMode mode, closestMode;
        SDL_GetCurrentDisplayMode(winManager.targetDisplay(), &mode);
        mode.w = w;
        mode.h = h;
        SDL_GetClosestDisplayMode(winManager.targetDisplay(), &mode, &closestMode);
        SDL_SetWindowDisplayMode(_mainWindow, &closestMode);
    } else {
        if (_externalResizeEvent) {
            // Find a decent resolution close to our dragged dimensions
            SDL_DisplayMode mode, closestMode;
            SDL_GetCurrentDisplayMode(winManager.targetDisplay(), &mode);
            mode.w = w;
            mode.h = h;
            SDL_GetClosestDisplayMode(winManager.targetDisplay(), &mode, &closestMode);
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
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    I32 winX = SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay());
    I32 winY = SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay());
    setPosition(type(), winX, winY);
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