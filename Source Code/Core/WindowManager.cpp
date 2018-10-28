#include "stdafx.h"

#include "Headers/WindowManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/EventHandler.h"

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

    bool validate(I32 errCode) {
        if (errCode != 0) {
            Console::errorfn(Locale::get(_ID("SDL_ERROR")), SDL_GetError());
            return false;
        }

        return true;
    };

    bool validateAssert(I32 errCode) {
        if (!validate(errCode)) {
            assert(errCode == 0);
            return false;
        }

        return true;
    };
}; // namespace 

hashMap<CursorStyle, SDL_Cursor*> WindowManager::s_cursors;

WindowManager::WindowManager()  noexcept 
   : _apiFlags(0),
     _mainWindowGUID(-1),
     _context(nullptr),
     _eventHandler(nullptr)
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
    if (_eventHandler != nullptr) {
        // Double init
        return ErrorCode::WINDOW_INIT_ERROR;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return ErrorCode::WINDOW_INIT_ERROR;
    }
    
    _context = &context;
    _eventHandler = std::make_unique<Input::EventHandler>(_context->kernel());

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

    // Toggle multi-sampling if requested.
    I32 msaaSamples = to_I32(_context->config().rendering.msaaSamples);
    I32 shadowSamples = to_I32(_context->config().rendering.shadowMapping.msaaSamples);

    I32 maxSamples = 0;
    if (msaaSamples > 0 || shadowSamples > 0) {
        maxSamples = std::max(msaaSamples, shadowSamples);
        if (validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1))) {
            while (!validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, maxSamples))) {
                maxSamples = maxSamples / 2;
                if (maxSamples == 0) {
                    validate(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0));
                    break;
                }
            }
        } else {
            maxSamples = 0;
        }
    }

    if (maxSamples < msaaSamples) {
        _context->config().rendering.msaaSamples = to_U8(maxSamples);
    }
    if (maxSamples < shadowSamples) {
        _context->config().rendering.shadowMapping.msaaSamples = to_U8(maxSamples);
    }


    WindowDescriptor descriptor = {};
    descriptor.position = initialPosition;
    descriptor.dimensions = initialResolution;
    descriptor.targetDisplay = displayIndex;
    descriptor.title = _context->config().title;
    descriptor.externalClose = false;
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
    DisplayWindow* window = createWindow(descriptor, err);

    if (err == ErrorCode::NO_ERR) {
        _mainWindowGUID = window->getGUID();

        window->addEventListener(WindowEvent::MINIMIZED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(true);
            return true;
        });
        window->addEventListener(WindowEvent::MAXIMIZED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(false);
            return true;
        });
        window->addEventListener(WindowEvent::RESTORED, [this](const DisplayWindow::WindowEventArgs& args) {
            ACKNOWLEDGE_UNUSED(args);
            _context->app().mainLoopPaused(false);
            return true;
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

DisplayWindow* WindowManager::createWindow(const WindowDescriptor& descriptor, ErrorCode& err, U32& windowIndex ) {
    windowIndex = std::numeric_limits<U32>::max();

    DisplayWindow* window = MemoryManager_NEW DisplayWindow(*this, *_context);
    assert(window != nullptr);

    windowIndex = to_U32(_windows.size());
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

    if (err == ErrorCode::NO_ERR) {
        err = configureAPISettings(descriptor.flags);
    }

    err = window->init(mainWindowFlags,
                       fullscreen ? WindowType::FULLSCREEN : WindowType::WINDOW,
                       descriptor);

    window->clearColour(descriptor.clearColour,
                        BitCompare(descriptor.flags, WindowDescriptor::Flags::CLEAR_COLOUR),
                        BitCompare(descriptor.flags, WindowDescriptor::Flags::CLEAR_DEPTH));

    if (err == ErrorCode::NO_ERR) {
        err = applyAPISettings(window, descriptor.flags);
    }

    if (err != ErrorCode::NO_ERR) {
        _windows.pop_back();
        MemoryManager::SAFE_DELETE(window);
    }

    window->addEventListener(WindowEvent::SIZE_CHANGED, [this](const DisplayWindow::WindowEventArgs& args) {
        SizeChangeParams params;
        params.width = to_U16(args.x);
        params.height = to_U16(args.y);
        params.isWindowResize = true;
        params.isFullScreen = args._flag;
        params.winGUID = args._windowGUID;

        // Only if rendering window
        _context->app().onSizeChange(params);
        return true;
    });

    if (!descriptor.externalClose) {
        window->addEventListener(WindowEvent::CLOSE_REQUESTED, [this](const DisplayWindow::WindowEventArgs& args) {
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
                return false;
            }
            return true;
        });
    }
    return window;
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

        if(event.type == SDL_QUIT) {
            _context->app().RequestShutdown();
        } else {
            onSDLInputEvent(event);
        }
    };
}

void WindowManager::onSDLInputEvent(SDL_Event event) {
    // Find the window that sent the event
    DisplayWindow* eventWindow = nullptr;
    for (DisplayWindow* win : _windows) {
        if (win->_windowID == event.window.windowID) {
            eventWindow = win;
            break;
        }
    }

    if (eventWindow == nullptr) {
        return;
    }

    switch (event.type) {
        //case SDL_TEXTEDITING:
        case SDL_TEXTINPUT: {
            Input::UTF8Event arg(eventWindow, 0, event.text.text);
            _eventHandler->onUTF8(arg);
        } break;

        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            Input::KeyEvent arg(eventWindow, 0);
            arg._key = Input::KeyCodeFromSDLKey(event.key.keysym.sym);
            arg._pressed = event.type == SDL_KEYDOWN;
            arg._text = nullptr;// SDL_GetKeyName(event.key.keysym.sym);
            arg._isRepeat = event.key.repeat;

            if ((event.key.keysym.mod & KMOD_LSHIFT) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::LSHIFT);
            }
            if ((event.key.keysym.mod & KMOD_RSHIFT) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::RSHIFT);
            }
            if ((event.key.keysym.mod & KMOD_LCTRL) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::LCTRL);
            }
            if ((event.key.keysym.mod & KMOD_RCTRL) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::RCTRL);
            }
            if ((event.key.keysym.mod & KMOD_LALT) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::LALT);
            }
            if ((event.key.keysym.mod & KMOD_RALT) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::RALT);
            }
            if ((event.key.keysym.mod & KMOD_LGUI) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::LGUI);
            }
            if ((event.key.keysym.mod & KMOD_RGUI) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::RGUI);
            }
            if ((event.key.keysym.mod & KMOD_NUM) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::NUM);
            }
            if ((event.key.keysym.mod & KMOD_CAPS) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::CAPS);
            }
            if ((event.key.keysym.mod & KMOD_MODE) != 0) {
                arg._modMask |= to_base(Input::KeyModifier::MODE);
            }
            if (arg._pressed) {
                _eventHandler->onKeyDown(arg);
            } else {
                _eventHandler->onKeyUp(arg);
            }
        } break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            Input::MouseButtonEvent arg(eventWindow, to_U8(event.button.which));
            arg.pressed = event.type == SDL_MOUSEBUTTONDOWN;
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    arg.button = Input::MouseButton::MB_Left;
                    break;
                case SDL_BUTTON_RIGHT:
                    arg.button = Input::MouseButton::MB_Right;
                    break;
                case SDL_BUTTON_MIDDLE:
                    arg.button = Input::MouseButton::MB_Middle;
                    break;
                case SDL_BUTTON_X1:
                    arg.button = Input::MouseButton::MB_Button3;
                    break;
                case SDL_BUTTON_X2:
                    arg.button = Input::MouseButton::MB_Button4;
                    break;
                case 6:
                    arg.button = Input::MouseButton::MB_Button5;
                    break;
                case 7:
                    arg.button = Input::MouseButton::MB_Button6;
                    break;
                case 8:
                    arg.button = Input::MouseButton::MB_Button7;
                    break;
            };
            arg.numCliks = to_U8(event.button.clicks);
            arg.relPosition.set(event.button.x, event.button.y);

            if (arg.pressed) {
                _eventHandler->mouseButtonPressed(arg);
            } else {
                _eventHandler->mouseButtonReleased(arg);
            }
        }break;
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEMOTION:
        {
            Input::MouseState state = {};
            state.X.abs = event.motion.x;
            state.X.rel = event.motion.xrel;
            state.Y.abs = event.motion.y;
            state.Y.rel = event.motion.yrel;
            if (event.type == SDL_MOUSEWHEEL) {
                state.HWheel = event.wheel.x;
                state.VWheel = event.wheel.y;
            } else {
                state.HWheel = state.VWheel = 0;
            }

            Input::MouseMoveEvent arg(eventWindow, to_U8(event.motion.which), state);
            _eventHandler->mouseMoved(arg);
        }break;
        case SDL_CONTROLLERAXISMOTION:
        case SDL_JOYAXISMOTION:
        {
            Input::JoystickData jData = {};
            jData._gamePad = event.type == SDL_CONTROLLERAXISMOTION;
            jData._data = jData._gamePad ? event.caxis.value : event.jaxis.value;
            
            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::AXIS_MOVE;
            element._data = jData;
            element._elementIndex = jData._gamePad ? event.caxis.axis : event.jaxis.axis;

            Input::JoystickEvent arg(eventWindow, to_U8(jData._gamePad ? event.caxis.which : event.jaxis.which));
            arg._element = element;

            _eventHandler->joystickAxisMoved(arg);

        }break;
        case SDL_JOYBALLMOTION:
        {
            Input::JoystickData jData = {};
            jData._smallData[0] = event.jball.xrel;
            jData._smallData[1] = event.jball.yrel;
            
            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::BALL_MOVE;
            element._data = jData;
            element._elementIndex = event.jball.ball;

            Input::JoystickEvent arg(eventWindow, to_U8(event.jball.which));
            arg._element = element;

            _eventHandler->joystickBallMoved(arg);
        }break;
        case SDL_JOYHATMOTION:
        {
            // POV
            U32 PovMask = 0;
            switch (event.jhat.value) {
                case SDL_HAT_CENTERED:
                    PovMask = to_base(Input::JoystickPovDirection::CENTERED);
                    break;
                case SDL_HAT_UP:
                    PovMask = to_base(Input::JoystickPovDirection::UP);
                    break;
                case SDL_HAT_RIGHT:
                    PovMask = to_base(Input::JoystickPovDirection::RIGHT);
                    break;
                case SDL_HAT_DOWN:
                    PovMask = to_base(Input::JoystickPovDirection::DOWN);
                    break;
                case SDL_HAT_LEFT:
                    PovMask = to_base(Input::JoystickPovDirection::LEFT);
                    break;
                case SDL_HAT_RIGHTUP:
                    PovMask = to_base(Input::JoystickPovDirection::RIGHT) |
                              to_base(Input::JoystickPovDirection::UP);
                    break;
                case SDL_HAT_RIGHTDOWN:
                    PovMask = to_base(Input::JoystickPovDirection::RIGHT) |
                              to_base(Input::JoystickPovDirection::DOWN);
                    break;
                case SDL_HAT_LEFTUP:
                    PovMask = to_base(Input::JoystickPovDirection::LEFT) |
                              to_base(Input::JoystickPovDirection::UP);
                    break;
                case SDL_HAT_LEFTDOWN:
                    PovMask = to_base(Input::JoystickPovDirection::LEFT) |
                              to_base(Input::JoystickPovDirection::DOWN);
                    break;
            };

            Input::JoystickData jData = {};
            jData._data = PovMask;

            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::POV_MOVE;
            element._data = jData;
            element._elementIndex = event.jhat.hat;

            Input::JoystickEvent arg(eventWindow, to_U8(event.jhat.which));
            arg._element = element;

            _eventHandler->joystickPovMoved(arg);
        }break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            Input::JoystickData jData = {};
            jData._gamePad = event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP;

            Uint8 state = jData._gamePad ? event.cbutton.state : event.jbutton.state;
            jData._data = state == SDL_PRESSED ? to_U32(Input::InputState::PRESSED) : to_U32(Input::InputState::RELEASED);
            
            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::BUTTON_PRESS;
            element._data = jData;
            element._elementIndex = jData._gamePad ? event.cbutton.button : event.jbutton.button;
            
            Input::JoystickEvent arg(eventWindow, to_U8(jData._gamePad  ? event.cbutton.which : event.jbutton.which));
            arg._element = element;

            if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONDOWN) {
                _eventHandler->joystickButtonPressed(arg);
            } else {
                _eventHandler->joystickButtonReleased(arg);
            }
        }break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEADDED:
        case SDL_JOYDEVICEREMOVED:
        {
            Input::JoystickData jData = {};
            jData._gamePad = event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED;
            jData._data = (jData._gamePad ? event.type == SDL_CONTROLLERDEVICEADDED : event.type == SDL_JOYDEVICEADDED) ? 1 : 0;

            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::JOY_ADD_REMOVE;
            element._data = jData;

            Input::JoystickEvent arg(eventWindow, to_U8(jData._gamePad ? event.cdevice.which : event.jdevice.which));
            arg._element = element;

            _eventHandler->joystickAddRemove(arg);
        }break;
        
       
        case SDL_CONTROLLERDEVICEREMAPPED:
        {
            
            Input::JoystickElement element = {};
            element._type = Input::JoystickElementType::JOY_REMAP;

            Input::JoystickEvent arg(eventWindow, to_U8(event.jdevice.which));
            arg._element = element;

            _eventHandler->joystickRemap(arg);
        }break;
    }
}

bool WindowManager::anyWindowFocus() const {
    return getFocusedWindow() != nullptr;
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

    if (BitCompare(window->_flags, WindowFlags::OWNS_RENDER_CONTEXT)) {
        SDL_GL_DeleteContext((SDL_GLContext)window->_userData);
        window->_userData = nullptr;
    }
}

ErrorCode WindowManager::configureAPISettings(U32 descriptorFlags) {
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

    if (!Config::ENABLE_GPU_VALIDATION) {
        //validateAssert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR, 1));
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
    if (BitCompare(descriptorFlags, to_base(WindowDescriptor::Flags::SHARE_CONTEXT))) {
        validate(SDL_GL_MakeCurrent(getMainWindow().getRawWindow(), (SDL_GLContext)getMainWindow().userData()));
    }

    return ErrorCode::NO_ERR;
}

ErrorCode WindowManager::applyAPISettings(DisplayWindow* window, U32 descriptorFlags) {
    // Create a context and make it current
    if (BitCompare(window->_flags, WindowFlags::OWNS_RENDER_CONTEXT)) {
        window->_userData = SDL_GL_CreateContext(window->getRawWindow());
    } else {
        window->_userData = getMainWindow().userData();
    }

    if (window->_userData == nullptr) {
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

    // Switch back to the main render context
    if (BitCompare(window->_flags, WindowFlags::OWNS_RENDER_CONTEXT)) {
        validate(SDL_GL_MakeCurrent(getMainWindow().getRawWindow(), (SDL_GLContext)getMainWindow().userData()));
    }

    return ErrorCode::NO_ERR;
}

void WindowManager::captureMouse(bool state) {
    SDL_CaptureMouse(state ? SDL_TRUE : SDL_FALSE);
}

void WindowManager::setCursorPosition(I32 x, I32 y, bool global) {
    if (!global) {
        DisplayWindow* focusedWindow = getFocusedWindow();
        if (focusedWindow != nullptr) {
            if (x == -1) {
                x = SDL_WINDOWPOS_CENTERED_DISPLAY(focusedWindow->currentDisplayIndex());
            }
            if (y == -1) {
                y = SDL_WINDOWPOS_CENTERED_DISPLAY(focusedWindow->currentDisplayIndex());
            }

            focusedWindow->setCursorPosition(x, y);
        } else {
            setCursorPosition(x, y, true);
        }
    } else {
        if (x == -1) {
            x = SDL_WINDOWPOS_CENTERED_DISPLAY(getMainWindow().currentDisplayIndex());
        }
        if (y == -1) {
            y = SDL_WINDOWPOS_CENTERED_DISPLAY(getMainWindow().currentDisplayIndex());
        }

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
    DisplayWindow* focusedWindow = getFocusedWindow();
    if (focusedWindow != nullptr) {
        const vec2<U16>& center = focusedWindow->getDimensions();
        setCursorPosition(to_I32(center.x * 0.5f), to_I32(center.y * 0.5f));
    } else {
        setCursorPosition(-1, -1, true);
    }
}

void WindowManager::hideAll() {
    for (DisplayWindow* win : _windows) {
        win->hidden(true);
    }
}

}; //namespace Divide