#include "Headers/WindowManager.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

WindowManager::WindowManager() : _displayIndex(0),
                                 _activeWindowGUID(-1)
{
    SDL_Init(SDL_INIT_VIDEO);
}

ErrorCode WindowManager::init(RenderAPI api,
                              ResolutionByType initialResolutions,
                              bool startFullScreen,
                              I32 targetDisplayIndex)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return ErrorCode::WINDOW_INIT_ERROR;
    }

    targetDisplay(std::max(std::min(targetDisplayIndex, SDL_GetNumVideoDisplays() - 1), 0));

    // Most runtime variables are stored in the ParamHandler, including
    // initialization settings retrieved from XML
    SysInfo& systemInfo = Application::getInstance().getSysInfo();
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(targetDisplay(), &displayMode);
    systemInfo._systemResolutionWidth = displayMode.w;
    systemInfo._systemResolutionHeight = displayMode.h;

    initialResolutions[to_const_uint(WindowType::FULLSCREEN)].set(to_ushort(displayMode.w),
                                                                  to_ushort(displayMode.h));
    initialResolutions[to_const_uint(WindowType::FULLSCREEN_WINDOWED)].set(to_ushort(displayMode.w),
                                                                           to_ushort(displayMode.h));

    ErrorCode err = initWindow(0, createAPIFlags(api), initialResolutions, startFullScreen, targetDisplayIndex);

    if (err == ErrorCode::NO_ERR) {
        setActiveWindow(0);
        GPUState& gState = GFX_DEVICE.gpuState();
        // Query available display modes (resolution, bit depth per channel and
        // refresh rates)
        I32 numberOfDisplayModes = 0;
        I32 numDisplays = SDL_GetNumVideoDisplays();
        for (I32 display = 0; display < numDisplays; ++display) {
            numberOfDisplayModes = SDL_GetNumDisplayModes(display);
            for (I32 mode = 0; mode < numberOfDisplayModes; ++mode) {
                SDL_GetDisplayMode(display, mode, &displayMode);
                // Register the display modes with the GFXDevice object
                GPUState::GPUVideoMode tempDisplayMode;
                for (I32 i = 0; i < numberOfDisplayModes; ++i) {
                    tempDisplayMode._resolution.set(displayMode.w, displayMode.h);
                    tempDisplayMode._bitDepth = SDL_BITSPERPIXEL(displayMode.format);
                    tempDisplayMode._formatName = SDL_GetPixelFormatName(displayMode.format);
                    tempDisplayMode._refreshRate.push_back(to_ubyte(displayMode.refresh_rate));
                    Util::ReplaceStringInPlace(tempDisplayMode._formatName, "SDL_PIXELFORMAT_", "");
                    gState.registerDisplayMode(to_ubyte(display), tempDisplayMode);
                    tempDisplayMode._refreshRate.clear();
                }
            }
        }
    }

    return err;
}

void WindowManager::close() {
    for (DisplayWindow& window : _windows) {
        window.destroyWindow();
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

ErrorCode WindowManager::initWindow(U32 index,
                                    U32 windowFlags,
                                    ResolutionByType initialResolutions,
                                    bool startFullScreen,
                                    I32 targetDisplayIndex) {
    index = std::min(index, to_uint(_windows.size() - 1));

    return _windows[index].init(windowFlags,
                                startFullScreen ? WindowType::FULLSCREEN
                                                : WindowType::WINDOW,
                                initialResolutions);
}

void WindowManager::setActiveWindow(U32 index) {
    index = std::min(index, to_uint(_windows.size() -1));
    _activeWindowGUID = _windows[index].getGUID();
    SysInfo& systemInfo = Application::getInstance().getSysInfo();
    getWindowHandle(_windows[index].getRawWindow(), systemInfo);
}

U32 WindowManager::createAPIFlags(RenderAPI api) {
    ParamHandler& par = ParamHandler::getInstance();

    U32 windowFlags = 0;

    if (api == RenderAPI::OpenGL || api == RenderAPI::OpenGLES) {
        Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#      if defined(ENABLE_GPU_VALIDATION)
            // OpenGL error handling is available in any build configuration
            // if the proper defines are in place.
            OpenGLFlags |= SDL_GL_CONTEXT_DEBUG_FLAG |
                           SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG |
                           SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;
#       endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, OpenGLFlags);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        // 32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        // Toggle multi-sampling if requested.
        // This options requires a client-restart, sadly.
        I32 msaaSamples = par.getParam<I32>("rendering.MSAAsampless", 0);
        if (msaaSamples > 0) {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaSamples);
        }

        // OpenGL ES is not yet supported, but when added, it will need to mirror
        // OpenGL functionality 1-to-1
        if (api == RenderAPI::OpenGLES) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        }  else {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        }

        windowFlags |= SDL_WINDOW_OPENGL;
    } 

    windowFlags |= SDL_WINDOW_HIDDEN;
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (par.getParam<bool>("runtime.windowResizable", true)) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    return windowFlags;
}

void WindowManager::setCursorPosition(I32 x, I32 y) const {
    getActiveWindow().setCursorPosition(x, y);
}

void WindowManager::handleWindowEvent(WindowEvent event, I64 winGUID, I32 data1, I32 data2) {
    switch (event) {
        case WindowEvent::HIDDEN: {
        } break;
        case WindowEvent::SHOWN: {
        } break;
        case WindowEvent::MINIMIZED: {
            Application::getInstance().mainLoopPaused(true);
            getWindow(winGUID).minimized(true);
        } break;
        case WindowEvent::MAXIMIZED: {
            getWindow(winGUID).minimized(false);
        } break;
        case WindowEvent::RESTORED: {
            getWindow(winGUID).minimized(false);
        } break;
        case WindowEvent::LOST_FOCUS: {
            getWindow(winGUID).hasFocus(false);
        } break;
        case WindowEvent::GAINED_FOCUS: {
            getWindow(winGUID).hasFocus(true);
        } break;
        case WindowEvent::RESIZED_INTERNAL: {
            // Only if rendering window
            if (_activeWindowGUID == winGUID) {
                Application::getInstance().onChangeWindowSize(to_ushort(data1), 
                                                              to_ushort(data2));
            }
        } break;
        case WindowEvent::RESIZED_EXTERNAL: {
        } break;
        case WindowEvent::APP_LOOP: {
            for (DisplayWindow& win : _windows) {
                win.update();
            }
        } break;
    };
}

}; //namespace Divide