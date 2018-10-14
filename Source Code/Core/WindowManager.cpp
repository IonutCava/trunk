#include "stdafx.h"

#include "Headers/WindowManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"

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

hashMap<CursorStyle, SDL_Cursor*> WindowManager::s_cursors;

WindowManager::WindowManager()  noexcept 
   : _apiFlags(0),
     _mainWindowGUID(-1),
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
                              const vec2<I16>& initialPosition,
                              const vec2<U16>& initialResolution,
                              bool startFullScreen,
                              I32 targetDisplayIndex)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return ErrorCode::WINDOW_INIT_ERROR;
    }
    
    _context = &context;
    RenderAPI api = _context->gfx().getAPI();

    _monitors.resize(0);
    I32 displayCount = SDL_GetNumVideoDisplays();
    for (I32 i = 0; i < displayCount; ++i) {
        MonitorData data = {};

        SDL_Rect r;
        SDL_GetDisplayBounds(i, &r);
        data.viewport.xy(to_U16(r.x), to_U16(r.y));
        data.viewport.zw(to_U16(r.w), to_U16(r.h));

        SDL_GetDisplayUsableBounds(i, &r);
        data.drawableArea.xy(to_U16(r.x), to_U16(r.y));
        data.drawableArea.zw(to_U16(r.w), to_U16(r.h));

        SDL_GetDisplayDPI(i, &data.dpi, nullptr, nullptr);

        _monitors.push_back(data);
    }

    I32 displayIndex = std::max(std::min(targetDisplayIndex, displayCount - 1), 0);

    SysInfo& systemInfo = sysInfo();
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
    systemInfo._systemResolutionWidth = displayMode.w;
    systemInfo._systemResolutionHeight = displayMode.h;

    _apiFlags = createAPIFlags(api);

    WindowDescriptor descriptor = {};
    descriptor.position = initialPosition;
    descriptor.dimensions = initialResolution;
    descriptor.targetDisplay = displayIndex;
    descriptor.title = _context->config().title;
    if (_context->config().runtime.enableVSync) {
        SetBit(descriptor.flags, WindowDescriptor::Flags::VSYNC);
    }

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
        _mainWindowGUID = _windows[idx]->getGUID();

        _windows[idx]->addEventListener(WindowEvent::MINIMIZED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(true);
        });
        _windows[idx]->addEventListener(WindowEvent::MAXIMIZED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(false);
        });
        _windows[idx]->addEventListener(WindowEvent::RESTORED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(false);
        });

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

    for (U8 i = 0; i < to_U8(CursorStyle::COUNT); ++i) {
        CursorStyle style = static_cast<CursorStyle>(i);
        s_cursors[style] = SDL_CreateSystemCursor(CursorToSDL(style));
    }

    return err;
}

void WindowManager::close() {
    for (DisplayWindow* window : _windows) {
        window->destroyWindow();
    }
    _windows.clear();
    for (auto it : s_cursors) {
        SDL_FreeCursor(it.second);
    }
    s_cursors.clear();
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
        if (BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::ALWAYS_ON_TOP))) {
            mainWindowFlags |= SDL_WINDOW_ALWAYS_ON_TOP;
        }
        bool fullscreen = BitCompare(descriptor.flags, to_base(WindowDescriptor::Flags::FULLSCREEN));

        err = _windows[ret]->init(mainWindowFlags,
                                  fullscreen ? WindowType::FULLSCREEN : WindowType::WINDOW,
                                  descriptor);

        _windows[ret]->clearColour(descriptor.clearColour,
                                   BitCompare(descriptor.flags, WindowDescriptor::Flags::CLEAR_COLOUR),
                                   BitCompare(descriptor.flags, WindowDescriptor::Flags::CLEAR_DEPTH));

        if (err == ErrorCode::NO_ERR) {
            err = configureAPISettings(_windows[ret], descriptor.flags);
        }

        if (err != ErrorCode::NO_ERR) {
            _windows.pop_back();
            MemoryManager::SAFE_DELETE(window);
        }

        _windows[ret]->addEventListener(WindowEvent::RESIZED, [this](const DisplayWindow::WindowEventArgs& args) {
            SizeChangeParams params;
            params.width = to_U16(args.x);
            params.height = to_U16(args.y);
            params.isWindowResize = true;
            params.isFullScreen = args._flag;
            params.winGUID = args._windowGUID;

            // Only if rendering window
            _context->app().onSizeChange(params);
        });

        _windows[ret]->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) {
            Console::d_printfn(Locale::get(_ID("WINDOW_CLOSE_EVENT")), args._windowGUID);

            if (_mainWindowGUID == args._windowGUID) {
                _context->app().RequestShutdown();
            } else {
                for (DisplayWindow*& win : _windows) {
                    if (win->getGUID() == args._windowGUID) {
                        if (!destroyWindow(win)) {
                            Console::errorfn(Locale::get(_ID("WINDOW_CLOSE_EVENT_ERROR")), args._windowGUID);
                            win->hidden(true);
                        }
                        break;
                    }
                }
            }
        });
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
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_WINDOWEVENT) {
            // Pass the window event to the proper window
            for (DisplayWindow* win : _windows) {
                if (win->_windowID == event.window.windowID) {
                    win->handleEvent(event);
                    break;
                }
            }
            continue;
        }

        switch(event.type) {
            case SDL_QUIT: {
                _context->app().RequestShutdown();
            } break;
            case SDL_TEXTINPUT: {
                DisplayWindow& focusedWindow = getFocusedWindow();
                DisplayWindow::WindowEventArgs args = {};
                args._windowGUID = focusedWindow.getGUID();
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
    return getFocusedWindow().hasFocus();
}

U32 WindowManager::createAPIFlags(RenderAPI api) {
    U32 windowFlags = 0;

    if (api == RenderAPI::OpenGL || api == RenderAPI::OpenGLES) {
        Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG |
                             SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;

        if (Config::ENABLE_GPU_VALIDATION) {
            // OpenGL error handling is available in any build configuration
            // if the proper defines are in place.
            OpenGLFlags |= SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG
                      /*|  SDL_GL_CONTEXT_DEBUG_FLAG*/;
        }
        windowFlags |= SDL_WINDOW_OPENGL;
    } 

    return windowFlags;
}

void WindowManager::destroyAPISettings(DisplayWindow* window) {
    if (!window || !BitCompare(SDL_GetWindowFlags(window->getRawWindow()), to_U32(SDL_WINDOW_OPENGL))) {
        return;
    }

    SDL_GL_DeleteContext((SDL_GLContext)window->_userData);
}

ErrorCode WindowManager::configureAPISettings(DisplayWindow* window, U32 descriptorFlags) {
    if (!window || !BitCompare(SDL_GetWindowFlags(window->getRawWindow()), to_U32(SDL_WINDOW_OPENGL))) {
        return ErrorCode::GFX_NOT_SUPPORTED;
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

    Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;

    if (Config::ENABLE_GPU_VALIDATION) {
        // OpenGL error handling is available in any build configuration if the proper defines are in place.
        OpenGLFlags |= SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG /*|  SDL_GL_CONTEXT_DEBUG_FLAG*/;
    }

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

    bool shareGLContext = BitCompare(descriptorFlags, to_base(WindowDescriptor::Flags::SHARE_CONTEXT));

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

    if (msaaSamples == 0 && _context->config().rendering.shadowMapping.msaaSamples > 0) {
        validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1));
    }

    // OpenGL ES is not yet supported, but when added, it will need to mirror
    // OpenGL functionality 1-to-1
    if (_context->gfx().getAPI() == RenderAPI::OpenGLES) {
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1));
    } else {
        validate(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
        validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4));
        if (true || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6) != 0) {
            validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5));
        }
    }

    validateAssert(SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1));

    if (shareGLContext) {

        validate(SDL_GL_MakeCurrent(getMainWindow().getRawWindow(), (SDL_GLContext)getMainWindow().userData()));
    }

    // Create a context and make it current
    window->_userData = SDL_GL_CreateContext(window->getRawWindow());

    if (window->_userData == nullptr)
    {
        Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE")), SDL_GetError());
        Console::printfn(Locale::get(_ID("WARN_SWITCH_API")));
        Console::printfn(Locale::get(_ID("WARN_APPLICATION_CLOSE")));
        return ErrorCode::OGL_OLD_HARDWARE;
    }

    if (BitCompare(window->_flags, WindowFlags::VSYNC)) {
        // Vsync is toggled on or off via the external config file
        bool vsyncSet = false;
        // Late swap may fail
        if (_context->config().runtime.adaptiveSync) {
            vsyncSet = SDL_GL_SetSwapInterval(-1) != -1;
        }

        if (!vsyncSet) {
            vsyncSet = SDL_GL_SetSwapInterval(1) != -1;
        }
        assert(vsyncSet);
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    if (shareGLContext) {
        validate(SDL_GL_MakeCurrent(getMainWindow().getRawWindow(), (SDL_GLContext)getMainWindow().userData()));
    }

    return ErrorCode::NO_ERR;
}

void WindowManager::captureMouse(bool state) {
    SDL_CaptureMouse(state ? SDL_TRUE : SDL_FALSE);
}

void WindowManager::setCursorPosition(I32 x, I32 y, bool global) {
    if (!global) {
        getFocusedWindow().setCursorPosition(x, y);
    } else {
        SDL_WarpMouseGlobal(x, y);
    }
    Attorney::KernelWindowManager::setCursorPosition(_context->app().kernel(), x, y);
}

void WindowManager::setCursorStyle(CursorStyle style) {
    SDL_SetCursor(s_cursors[style]);
}

vec2<I32> WindowManager::getCursorPosition(bool global) {
    vec2<I32> ret(-1);
    getMouseState(ret, global);
    return ret;
}

U32 WindowManager::getMouseState(vec2<I32>& pos, bool global) {
    if (global) {
        return (U32)SDL_GetGlobalMouseState(&pos.x, &pos.y);
    }
    
    return (U32)SDL_GetMouseState(&pos.x, &pos.y);
}

void WindowManager::setCaptureMouse(bool state) {
    SDL_CaptureMouse(state ? SDL_TRUE : SDL_FALSE);
}

void WindowManager::snapCursorToCenter() {
    const vec2<U16>& center = getFocusedWindow().getDimensions();
    setCursorPosition(to_I32(center.x * 0.5f), to_I32(center.y * 0.5f));
}

void WindowManager::hideAll() {
    for (DisplayWindow* win : _windows) {
        win->hidden(true);
    }
}

}; //namespace Divide