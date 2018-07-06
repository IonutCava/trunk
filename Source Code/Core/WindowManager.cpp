#include "stdafx.h"

#include "Headers/WindowManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

WindowManager::WindowManager() : _displayIndex(0),
                                 _activeWindowGUID(-1),
                                 _context(nullptr)
{
    SDL_Init(SDL_INIT_VIDEO);
}

WindowManager::~WindowManager()
{
    MemoryManager::DELETE_VECTOR(_windows);
}

ErrorCode WindowManager::init(PlatformContext& context,
                              RenderAPI api,
                              ResolutionByType initialResolutions,
                              bool startFullScreen,
                              I32 targetDisplayIndex)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return ErrorCode::WINDOW_INIT_ERROR;
    }

    _context = &context;

    targetDisplay(std::max(std::min(targetDisplayIndex, SDL_GetNumVideoDisplays() - 1), 0));

    SysInfo& systemInfo = sysInfo();
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(targetDisplay(), &displayMode);
    systemInfo._systemResolutionWidth = displayMode.w;
    systemInfo._systemResolutionHeight = displayMode.h;

    initialResolutions[to_base(WindowType::FULLSCREEN)].set(to_U16(displayMode.w),
                                                                  to_U16(displayMode.h));
    initialResolutions[to_base(WindowType::FULLSCREEN_WINDOWED)].set(to_U16(displayMode.w),
                                                                           to_U16(displayMode.h));

    _windows.emplace_back(MemoryManager_NEW DisplayWindow(*this, context));

    ErrorCode err = initWindow(0,
                               createAPIFlags(api),
                               initialResolutions,
                               startFullScreen,
                               targetDisplayIndex,
                               _context->config().title.c_str());

    if (err == ErrorCode::NO_ERR) {
        setActiveWindow(0);
        GPUState& gState = _context->gfx().gpuState();
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
                    tempDisplayMode._refreshRate.push_back(to_U8(displayMode.refresh_rate));
                    Util::ReplaceStringInPlace(tempDisplayMode._formatName, "SDL_PIXELFORMAT_", "");
                    gState.registerDisplayMode(to_U8(display), tempDisplayMode);
                    tempDisplayMode._refreshRate.clear();
                }
            }
        }
    }

    return err;
}

void WindowManager::close() {
    for (DisplayWindow* window : _windows) {
        window->destroyWindow();
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

ErrorCode WindowManager::initWindow(U32 index,
                                    U32 windowFlags,
                                    const ResolutionByType& initialResolutions,
                                    bool startFullScreen,
                                    I32 targetDisplayIndex,
                                    const char* windowTitle) {
    index = std::min(index, to_U32(_windows.size() - 1));

    return _windows[index]->init(windowFlags,
                                 startFullScreen ? WindowType::FULLSCREEN
                                                 : WindowType::WINDOW,
                                 initialResolutions,
                                 windowTitle);
}

bool WindowManager::destroyWindow(DisplayWindow*& window) {
    SDL_HideWindow(window->getRawWindow());
    if (window->destroyWindow() == ErrorCode::NO_ERR) {
        I64 targetGUID = window->getGUID();
        _windows.erase(
            std::remove_if(std::begin(_windows), std::end(_windows),
                           [&targetGUID](DisplayWindow* window)
                               -> bool { return window->getGUID() == targetGUID;}),
            std::end(_windows));
        window = nullptr;
        return true;
    }
    return false;
}

void WindowManager::setActiveWindow(U32 index) {
    index = std::min(index, to_U32(_windows.size() -1));
    _activeWindowGUID = _windows[index]->getGUID();
    SysInfo& systemInfo = sysInfo();
    getWindowHandle(_windows[index]->getRawWindow(), systemInfo);
}

U32 WindowManager::createAPIFlags(RenderAPI api) {
    U32 windowFlags = 0;

    if (api == RenderAPI::OpenGL || api == RenderAPI::OpenGLES) {
        Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
        if (Config::ENABLE_GPU_VALIDATION) {
            // OpenGL error handling is available in any build configuration
            // if the proper defines are in place.
            OpenGLFlags |= SDL_GL_CONTEXT_DEBUG_FLAG |
                           SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG |
                           SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;
        } else {
            // These seem to use the same value
            auto KHR_NO_ERROR = SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;
            OpenGLFlags |= KHR_NO_ERROR;
        }

        auto validate = [](I32 errCode) -> bool {
            if (errCode != 0) {
                Console::errorfn(Locale::get(_ID("SDL_ERROR")), SDL_GetError());
                return false;
            }

            return true;
        };

        auto validateAssert = [&validate](I32 errCode) -> bool {
            if (!validate(errCode)) {
                assert(errCode == 0);
                return false;
            }

            return true;
        };

        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, OpenGLFlags));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, SDL_GL_CONTEXT_RELEASE_BEHAVIOR_NONE));

        validateAssert(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
        // 32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
        validateAssert(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1));
        // Toggle multi-sampling if requested.
        // This options requires a client-restart, sadly.
        I32 msaaSamples = to_I32(_context->config().rendering.msaaSamples);
        if (msaaSamples > 0) {
            if (validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1))) {
                while (!validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaSamples))) {
                    msaaSamples = msaaSamples / 2;
                    if (msaaSamples == 0) {
                        validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0));
                        break;
                    }
                }
            } else {
                msaaSamples = 0;
            }
        }
        _context->config().rendering.msaaSamples = to_U8(msaaSamples);

        // OpenGL ES is not yet supported, but when added, it will need to mirror
        // OpenGL functionality 1-to-1
        if (api == RenderAPI::OpenGLES) {
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1));
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES));
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1));
        }  else {
            validate(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4));
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5));
        }

        windowFlags |= SDL_WINDOW_OPENGL;
    } 

    windowFlags |= SDL_WINDOW_HIDDEN;
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (_context->config().runtime.windowResizable) {
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
            _context->app().mainLoopPaused(true);
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
                _context->app().onChangeWindowSize(to_U16(data1), to_U16(data2));
            }
        } break;
        case WindowEvent::RESIZED_EXTERNAL: {
        } break;
        case WindowEvent::RESOLUTION_CHANGED: {
            // Only if rendering window
            if (_activeWindowGUID == winGUID) {
                _context->app().onChangeRenderResolution(to_U16(data1), to_U16(data2));
            }
        } break;
        case WindowEvent::APP_LOOP: {
            for (DisplayWindow* win : _windows) {
                win->update();
            }
        } break;
        case WindowEvent::CLOSE_REQUESTED: {
            Console::d_printfn(Locale::get(_ID("WINDOW_CLOSE_EVENT")), winGUID);

            if (_activeWindowGUID == winGUID) {
                _context->app().RequestShutdown();
            } else {
                for (DisplayWindow* win : _windows) {
                    if (win->getGUID() == winGUID) {
                        if (!destroyWindow(win)) {
                            Console::errorfn(Locale::get(_ID("WINDOW_CLOSE_EVENT_ERROR")), winGUID);
                            if (win != nullptr) {
                                SDL_HideWindow(win->getRawWindow());
                            }
                        }
                        //Iterator is now invalidated!
                        break;
                    }
                }

               
            }
        }
    };
}
}; //namespace Divide