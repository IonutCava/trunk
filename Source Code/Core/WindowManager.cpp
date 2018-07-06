#include "stdafx.h"

#include "Headers/WindowManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"


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
            case SDLK_ESCAPE: return Input::KeyCode::KC_ESCAPE;
            case SDLK_a: return Input::KeyCode::KC_A;
            case SDLK_c: return Input::KeyCode::KC_C;
            case SDLK_v: return Input::KeyCode::KC_V;
            case SDLK_x: return Input::KeyCode::KC_X;
            case SDLK_y: return Input::KeyCode::KC_Y;
            case SDLK_z: return Input::KeyCode::KC_Z;
        };
        return Input::KeyCode::KC_YEN;
    }
}


WindowManager::WindowManager() : _displayIndex(0),
                                 _apiFlags(0),
                                 _activeWindowGUID(-1),
                                 _mainWindowGUID(-1),
                                 _focusedWindowGUID(-1),
                                 _context(nullptr)
{
    SDL_Init(SDL_INIT_VIDEO);
}

WindowManager::~WindowManager()
{
    MemoryManager::DELETE_VECTOR(_windows);
}

vec2<U16> WindowManager::getFullscreenResolution() const {
    SysInfo& systemInfo = sysInfo();
    return vec2<U16>(systemInfo._systemResolutionWidth,
                     systemInfo._systemResolutionHeight);
}

ErrorCode WindowManager::init(PlatformContext& context,
                              RenderAPI api,
                              const vec2<U16>& initialResolution,
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

    _apiFlags = createAPIFlags(api);

    WindowDescriptor descriptor;
    descriptor.dimensions = initialResolution;
    descriptor.targetDisplay = targetDisplayIndex;
    descriptor.title = _context->config().title;

    SetBit(descriptor.flags, WindowDescriptor::Flags::HIDDEN);

    if (startFullScreen) {
        SetBit(descriptor.flags, WindowDescriptor::Flags::FULLSCREEN);
    }
    if (!_context->config().runtime.windowResizable) {
        ClearBit(descriptor.flags, WindowDescriptor::Flags::RESIZEABLE);
    }

    ErrorCode err = ErrorCode::NO_ERR;

    U32 idx = createWindow(descriptor, err);

    if (err == ErrorCode::NO_ERR) {
        setActiveWindow(idx);
        _mainWindowGUID = _windows[idx]->getGUID();
        _focusedWindowGUID = _mainWindowGUID;
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
    _windows.clear();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

U32 WindowManager::createWindow(const WindowDescriptor& descriptor, ErrorCode& err) {
    U32 ret = std::numeric_limits<U32>::max();

    DisplayWindow* window = MemoryManager_NEW DisplayWindow(*this, *_context);
    if (window != nullptr) {
        ret = to_U32(_windows.size());
        _windows.emplace_back(window);

        U32 mainWindowFlags = _apiFlags;
        if (BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::RESIZEABLE))) {
            mainWindowFlags |= SDL_WINDOW_RESIZABLE;
        }
        if (BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::ALLOW_HIGH_DPI))) {
            mainWindowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
        }
        if (BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::HIDDEN))) {
            mainWindowFlags |= SDL_WINDOW_HIDDEN;
        }
        if (!BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::DECORATED))) {
            mainWindowFlags |= SDL_WINDOW_BORDERLESS;
        }
        bool fullscreen = BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::FULLSCREEN));

        err = _windows[ret]->init(mainWindowFlags,
                                  fullscreen ? WindowType::FULLSCREEN : WindowType::WINDOW,
                                  descriptor.dimensions,
                                  descriptor.title.c_str());

        if (err != ErrorCode::NO_ERR) {
            _windows.pop_back();
            MemoryManager::SAFE_DELETE(window);
        }
        _windows[ret]->clearColour(descriptor.clearColour, true);
    }

    return ret;
}

bool WindowManager::destroyWindow(DisplayWindow*& window) {
    if (window == nullptr) {
        return true;
    }

    SDL_HideWindow(window->getRawWindow());
    I64 targetGUID = window->getGUID();
    if (window->destroyWindow() == ErrorCode::NO_ERR) {
        _windows.erase(
            std::remove_if(std::begin(_windows), std::end(_windows),
                           [&targetGUID](DisplayWindow* window)
                               -> bool { return window->getGUID() == targetGUID;}),
            std::end(_windows));
        MemoryManager::SAFE_DELETE(window);
        return true;
    }
    return false;
}

void WindowManager::update(const U64 deltaTimeUS) {
    pollSDLEvents();
    for (DisplayWindow* win : _windows) {
        win->update(deltaTimeUS);
    }
}

void WindowManager::pollSDLEvents() {
    static int s_KeyMod = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (Config::USE_ANT_TWEAK_BAR) {
            if (_context->config().gui.enableDebugVariableControls) {
                // send event to AntTweakBar
                if (TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION) != 0) {
                    continue;
                }
            }
        }

        if (event.type == SDL_WINDOWEVENT) {
            for (DisplayWindow* win : _windows) {
                win->handleEvent(event);
            }
            continue;
        }

        DisplayWindow& focusedWindow = getWindow(_focusedWindowGUID);
        DisplayWindow::WindowEventArgs args;
        args._windowGUID = _focusedWindowGUID;

        switch(event.type) {
            case SDL_QUIT: {
                handleWindowEvent(WindowEvent::APP_QUIT,
                                  -1,
                                  event.quit.type,
                                  event.quit.timestamp);
            } break;
            case SDL_TEXTINPUT: {
                args._text = event.text.text;
                focusedWindow.notifyListeners(WindowEvent::TEXT, args);
            } break;
             case SDL_KEYUP:
             case SDL_KEYDOWN:
             case SDL_MOUSEBUTTONDOWN: 
             case SDL_MOUSEBUTTONUP:
             case SDL_MOUSEMOTION:
             case SDL_MOUSEWHEEL:{
                 // OIS handles this
             } break;
        }
    };
}

bool WindowManager::anyWindowFocus() const {
    for (DisplayWindow* win : _windows) {
        if (win->hasFocus()) {
            return true;
        }
    }
    return false;
}
void WindowManager::setActiveWindow(U32 index) {
    index = std::min(index, to_U32(_windows.size() -1));
    _activeWindowGUID = _windows[index]->getGUID();
    *sysInfo()._focusedWindowHandle = _windows[index]->handle();
}

U32 WindowManager::createAPIFlags(RenderAPI api) {
    U32 windowFlags = 0;

    if (api == RenderAPI::OpenGL || api == RenderAPI::OpenGLES) {
        Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG |
                             SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;

        if (Config::ENABLE_GPU_VALIDATION) {
            // OpenGL error handling is available in any build configuration
            // if the proper defines are in place.
            OpenGLFlags |= SDL_GL_CONTEXT_DEBUG_FLAG |
                           SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG;
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
        if (!Config::ENABLE_GPU_VALIDATION) {
            //validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR, 1));
        }

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
            //if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6) != 0) {
                validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5));
            //}
        }

        windowFlags |= SDL_WINDOW_OPENGL;
    } 

    return windowFlags;
}

void WindowManager::setCursorPosition(I32 x, I32 y) const {
    getActiveWindow().setCursorPosition(x, y);
    Attorney::KernelWindowManager::setCursorPosition(_context->app().kernel(), x, y);
}

vec2<I32> WindowManager::getCursorPosition() const {
    return getActiveWindow().getCursorPosition();
}

void WindowManager::snapCursorToCenter() const {
    const vec2<U16>& center = getActiveWindow().getDimensions();
    setCursorPosition(to_I32(center.x * 0.5f), to_I32(center.y * 0.5f));
}

void WindowManager::handleWindowEvent(WindowEvent event, I64 winGUID, I32 data1, I32 data2, bool flag) {
    switch (event) {
        case WindowEvent::HIDDEN: {
        } break;
        case WindowEvent::SHOWN: {
        } break;
        case WindowEvent::MINIMIZED: {
            if (_mainWindowGUID == winGUID) {
                _context->app().mainLoopPaused(true);
            }
            getWindow(winGUID).minimized(true);
        } break;
        case WindowEvent::MAXIMIZED: {
            getWindow(winGUID).minimized(false);
        } break;
        case WindowEvent::RESTORED: {
            if (_mainWindowGUID == winGUID) {
                _context->app().mainLoopPaused(false);
            }

            getWindow(winGUID).minimized(false);
        } break;
        case WindowEvent::LOST_FOCUS: {
            getWindow(winGUID).hasFocus(false);
        } break;
        case WindowEvent::GAINED_FOCUS: {
            getWindow(winGUID).hasFocus(true);
            _focusedWindowGUID = winGUID;
        } break;
        case WindowEvent::RESIZED: {
            if (_mainWindowGUID == winGUID) {
                // Only if rendering window
                _context->app().onChangeWindowSize(to_U16(data1), to_U16(data2));
            }
            if (flag) {
                getWindow(winGUID).setDimensions(to_U16(data1),
                                                 to_U16(data2));
            }
        } break;
        case WindowEvent::RESOLUTION_CHANGED: {
            // Only if rendering window
            if (_mainWindowGUID == winGUID) {
                _context->app().onChangeRenderResolution(to_U16(data1), to_U16(data2));
            }
        } break;
        case WindowEvent::APP_LOOP: {
            //Nothing ... already handled
        } break;
        case WindowEvent::CLOSE_REQUESTED: {
            Console::d_printfn(Locale::get(_ID("WINDOW_CLOSE_EVENT")), winGUID);

            if (_mainWindowGUID == winGUID) {
                handleWindowEvent(WindowEvent::APP_QUIT, -1, -1, -1);
            } else {
                
                for (DisplayWindow*& win : _windows) {
                    if (win->getGUID() == winGUID) {
                        if (!destroyWindow(win)) {
                            Console::errorfn(Locale::get(_ID("WINDOW_CLOSE_EVENT_ERROR")), winGUID);
                            win->hidden(true);
                        }
                        break;
                    }
                }
            }
        } break;
        case WindowEvent::APP_QUIT: {
            for (DisplayWindow* win : _windows) {
                win->hidden(true);
            }
            _context->app().RequestShutdown();
        }break;
    };
}
}; //namespace Divide