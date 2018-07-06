#include "Headers/GLWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#endif //CEGUI_STATIC

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

#include <GLIM/glim.h>
#include <chrono>
#include <thread>

#include <SDL.h>

namespace Divide {

ErrorCode GL_API::createWindow() {
    ParamHandler& par = ParamHandler::getInstance();

    Uint32 OpenGLFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#if defined(_DEBUG) || defined(_GLDEBUG_IN_RELEASE)
    // OpenGL error handling is available in any build configuration
    // if the proper defines are in place.
    OpenGLFlags |= SDL_GL_CONTEXT_DEBUG_FLAG | 
                   SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG |
                   SDL_GL_CONTEXT_RESET_ISOLATION_FLAG;
#endif

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
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, par.getParam<I32>("rendering.MSAAsampless", 0));

    // OpenGL ES is not yet supported, but when added, it will need to mirror
    // OpenGL functionality 1-to-1
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::OpenGLES) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
#ifdef GL_VERSION_4_5
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
#endif
    }

    U32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (par.getParam<bool>("runtime.windowResizable", true)) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    WindowManager& winManager = Application::getInstance().getWindowManager();
    GLUtil::_mainWindow = SDL_CreateWindow(par.getParam<stringImpl>("appTitle", "Divide").c_str(),
                                           SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay()),
                                           SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay()),
                                           1,
                                           1,
                                           windowFlags);

    vec2<I32> position;
    SDL_GetWindowPosition(GLUtil::_mainWindow, &position.x, &position.y);
    winManager.setWindowPosition(vec2<U16>(position));

      // Check if we have a valid window
    if (!GLUtil::_mainWindow) {
        SDL_Quit();
        Console::errorfn(Locale::get("ERROR_GFX_DEVICE"),
                         Locale::get("ERROR_GL_OLD_VERSION"));
        Console::printfn(Locale::get("WARN_SWITCH_D3D"));
        Console::printfn(Locale::get("WARN_APPLICATION_CLOSE"));
        return ErrorCode::SDL_WINDOW_INIT_ERROR;
    }

    SysInfo& systemInfo = Application::getInstance().getSysInfo();
    getWindowHandle(GLUtil::_mainWindow, systemInfo);

    return ErrorCode::NO_ERR;
}

ErrorCode GL_API::createGLContext() {
    DIVIDE_ASSERT(GLUtil::_mainWindow != nullptr,
        "GL_API::createGLContext error: Tried to create an OpenGL context with no valid window!");

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    GLUtil::_glRenderContext = SDL_GL_CreateContext(GLUtil::_mainWindow);
    GLUtil::_glLoadingContext = SDL_GL_CreateContext(GLUtil::_mainWindow);

    return ErrorCode::NO_ERR;
}

ErrorCode GL_API::destroyWindow() {
    DIVIDE_ASSERT(GLUtil::_mainWindow != nullptr,
                  "GL_API::destroyWindow error: Tried to double-delete the same window!");

    SDL_DestroyWindow(GLUtil::_mainWindow);
    GLUtil::_mainWindow = nullptr;

    return ErrorCode::NO_ERR;
}

ErrorCode GL_API::destroyGLContext() {
    DIVIDE_ASSERT(GLUtil::_mainWindow != nullptr,
        "GL_API::destroyGLContext error: Tried to destroy the OpenGL context with no valid window!");
    SDL_GL_DeleteContext(GLUtil::_glRenderContext);
    SDL_GL_DeleteContext(GLUtil::_glLoadingContext);

    return ErrorCode::NO_ERR;
}

void GL_API::pollWindowEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT: {
                SDL_HideWindow(GLUtil::_mainWindow);
                Application::getInstance().RequestShutdown();
            } break;
            case SDL_WINDOWEVENT:  {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_ENTER:
                    case SDL_WINDOWEVENT_FOCUS_GAINED: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::GAINED_FOCUS,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_LEAVE:
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::LOST_FOCUS,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_RESIZED: {
                        _externalResizeEvent = true;
                        changeWindowSize(static_cast<U16>(event.window.data1),
                                         static_cast<U16>(event.window.data2));
                        _externalResizeEvent = false;
                    }break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::RESIZED_INTERNAL,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MOVED: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::MOVED,
                                                     event.window.data1,
                                                     event.window.data2);

                        if (!_internalMoveEvent) {
                            setWindowPosition(static_cast<U16>(event.window.data1),
                                              static_cast<U16>(event.window.data2));
                            _internalMoveEvent = false;
                        }
                    } break;
                    case SDL_WINDOWEVENT_SHOWN: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::SHOWN,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_HIDDEN: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::HIDDEN,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MINIMIZED: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::MAXIMIZED,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                    case SDL_WINDOWEVENT_MAXIMIZED: {
                        GFX_DEVICE.handleWindowEvent(WindowEvent::MAXIMIZED,
                                                     event.window.data1,
                                                     event.window.data2);
                    } break;
                };
            } break;
        }
    }
}

/// Try and create a valid OpenGL context taking in account the specified
/// resolution and command line arguments
ErrorCode GL_API::initRenderingAPI(GLint argc, char** argv) {
    // Fill our (abstract API <-> openGL) enum translation tables with proper
    // values
    GLUtil::fillEnumTables();
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        return ErrorCode::SDL_INIT_ERROR;
    }
    I32 numDisplays = SDL_GetNumVideoDisplays();
    WindowManager& winManager = Application::getInstance().getWindowManager();
    winManager.targetDisplay(std::max(std::min(winManager.targetDisplay(), numDisplays - 1), 0));

    // Most runtime variables are stored in the ParamHandler, including
    // initialization settings retrieved from XML
    ParamHandler& par = ParamHandler::getInstance();
    SysInfo& systemInfo = Application::getInstance().getSysInfo();
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(winManager.targetDisplay(), &displayMode);
    systemInfo._systemResolutionWidth = displayMode.w;
    systemInfo._systemResolutionHeight = displayMode.h;
    winManager.setWindowDimensions(WindowType::FULLSCREEN,
                                   vec2<U16>(displayMode.w, displayMode.h));
    winManager.setWindowDimensions(WindowType::FULLSCREEN_WINDOWED,
                                   vec2<U16>(displayMode.w, displayMode.h));

    ErrorCode errorState = createWindow();
    if (errorState != ErrorCode::NO_ERR) {
        return errorState;
    }

    errorState = createGLContext();
    if (errorState != ErrorCode::NO_ERR) {
        return errorState;
    }

    SDL_GL_MakeCurrent(GLUtil::_mainWindow, GLUtil::_glRenderContext);
    glbinding::Binding::initialize();
    glbinding::Binding::useCurrentContext();
    // Vsync is toggled on or off via the external config file
    SDL_GL_SetSwapInterval(par.getParam<bool>("runtime.enableVSync", false) ? 1 : 0);
        
    // If we got here, let's figure out what capabilities we have available
    // Maximum addressable texture image units in the fragment shader
    _maxTextureUnits = GLUtil::getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS);
    par.setParam<I32>("rendering.maxTextureSlots", _maxTextureUnits);
    // Maximum number of color attachments per framebuffer
    par.setParam<I32>("rendering.maxRenderTargetOutputs",
                      GLUtil::getIntegerv(GL_MAX_COLOR_ATTACHMENTS));
    // Query GPU vendor to enable/disable vendor specific features
    stringImpl gpuVendorByte;
    const char* gpuVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (gpuVendor != nullptr) {
        gpuVendorByte = gpuVendor;
    } else {
        gpuVendorByte = "unknown";
    }
    if (!gpuVendorByte.empty()) {
        if (gpuVendorByte.compare(0, 5, "Intel") == 0) {
            GFX_DEVICE.setGPUVendor(GPUVendor::INTEL);
        } else if (gpuVendorByte.compare(0, 6, "NVIDIA") == 0) {
            GFX_DEVICE.setGPUVendor(GPUVendor::NVIDIA);
        } else if (gpuVendorByte.compare(0, 3, "ATI") == 0 ||
                   gpuVendorByte.compare(0, 3, "AMD") == 0) {
            GFX_DEVICE.setGPUVendor(GPUVendor::AMD);
        }
    } else {
        gpuVendorByte = "Unknown GPU Vendor";
        GFX_DEVICE.setGPUVendor(GPUVendor::OTHER);
    }

    // Cap max anisotropic level to what the hardware supports
    par.setParam(
        "rendering.anisotropicFilteringLevel",
        std::min(
            GLUtil::getIntegerv(gl::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT),
            par.getParam<GLint>("rendering.anisotropicFilteringLevel", 1)));

    Console::printfn(Locale::get("GL_MAX_VERSION"),
                     GLUtil::getIntegerv(GL_MAJOR_VERSION),
                     GLUtil::getIntegerv(GL_MINOR_VERSION));

    // Number of sample buffers associated with the framebuffer & MSAA sample
    // count
    GLint samplerBuffers = GLUtil::getIntegerv(GL_SAMPLES);
    GLint sampleCount = GLUtil::getIntegerv(GL_SAMPLE_BUFFERS);
    Console::printfn(Locale::get("GL_MULTI_SAMPLE_INFO"), sampleCount,
                     samplerBuffers);
    // If we do not support MSAA on a hardware level for whatever reason,
    // override user set MSAA levels
    I32 msaaSamples = par.getParam<I32>("rendering.MSAAsampless", 0);
    if (samplerBuffers == 0 || sampleCount == 0) {
        msaaSamples = 0;
    }
    GFX_DEVICE.gpuState().initAA(static_cast<U8>(par.getParam<I32>("rendering.FXAAsamples", 0)),
                                 static_cast<U8>(msaaSamples));
    // Print all of the OpenGL functionality info to the console and log
    // How many uniforms can we send to fragment shaders
    Console::printfn(Locale::get("GL_MAX_UNIFORM"),
                     GLUtil::getIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS));
    // How many varying floats can we use inside a shader program
    Console::printfn(Locale::get("GL_MAX_FRAG_VARYING"),
                     GLUtil::getIntegerv(GL_MAX_VARYING_FLOATS));
    // How many uniforms can we send to vertex shaders
    Console::printfn(Locale::get("GL_MAX_VERT_UNIFORM"),
                     GLUtil::getIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS));
    // How many attributes can we send to a vertex shader
    Console::printfn(Locale::get("GL_MAX_VERT_ATTRIB"),
                     GLUtil::getIntegerv(GL_MAX_VERTEX_ATTRIBS));
    // Maximum number of texture units we can address in shaders
    Console::printfn(Locale::get("GL_MAX_TEX_UNITS"),
                     GLUtil::getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
                     par.getParam<I32>("rendering.maxTextureSlots", 16));
    // Query shading language version support
    Console::printfn(Locale::get("GL_GLSL_SUPPORT"),
                     glGetString(GL_SHADING_LANGUAGE_VERSION));
    // GPU info, including vendor, gpu and driver
    Console::printfn(Locale::get("GL_VENDOR_STRING"), gpuVendorByte.c_str(),
                     glGetString(GL_RENDERER), glGetString(GL_VERSION));
    // In order: Maximum number of uniform buffer binding points,
    //           maximum size in basic machine units of a uniform block and
    //           minimum required alignment for uniform buffer sizes and offset
    GLint uboffset = GLUtil::getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    Console::printfn(Locale::get("GL_UBO_INFO"),
                     GLUtil::getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS),
                     GLUtil::getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE) / 1024,
                     uboffset);
    par.setParam<I32>("rendering.UBOAligment", uboffset);
    // In order: Maximum number of shader storage buffer binding points,
    //           maximum size in basic machine units of a shader storage block,
    //           maximum total number of active shader storage blocks that may
    //           be accessed by all active shaders and
    //           minimum required alignment for shader storage buffer sizes and
    //           offset.
    GLint sboffset = GLUtil::getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    Console::printfn(
        Locale::get("GL_SSBO_INFO"),
        GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
        (GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE) / 1024) / 1024,
        GLUtil::getIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
        sboffset);
    par.setParam<I32>("rendering.SSBOAligment", sboffset);
    // Maximum number of subroutines and maximum number of subroutine uniform
    // locations usable in a shader
    Console::printfn(Locale::get("GL_SUBROUTINE_INFO"),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINES),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS));

    // OpenGL has a nifty error callback system, available in every build
    // configuration if required
#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    // GL_DEBUG_OUTPUT_SYNCHRONOUS is essential for debugging gl commands in the IDE
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // hardwire our debug callback function with OpenGL's implementation
    glDebugMessageCallback(GLUtil::DebugCallback, nullptr);
    // nVidia flushes a lot of useful info about buffer allocations and shader
    // recompiles due to state and what now, but those aren't needed until that's
    // what's actually causing the bottlenecks
    U32 nvidiaBufferErrors[] = { 131185, 131218 };
    // Disable shader compiler errors (shader class handles that)
    glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_TYPE_ERROR,
        GL_DONT_CARE, 0, nullptr, GL_FALSE);
    // Disable nVidia buffer allocation info (an easy enable is to change the
    // count param to 0)
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER,
        GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);
    // Shader will be recompiled nVidia error
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE,
        GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);
#endif

    // Set the clear color to the blue color used since the initial OBJ loader
    // days
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    // Seamless cubemaps are a nice feature to have enabled (core since 3.2)
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // Enable multisampling if we actually support and request it
    GFX_DEVICE.gpuState().MSAAEnabled() 
        ?  glEnable(GL_MULTISAMPLE) 
        :  glDisable(GL_MULTISAMPLE);

    // Line smoothing should almost always be used
    if (Config::USE_HARDWARE_AA_LINES) {
        glEnable(GL_LINE_SMOOTH);
        glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, &_lineWidthLimit);
    } else {
        glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, &_lineWidthLimit);
    }

    // Culling is enabled by default, but RenderStateBlocks can toggle it on a
    // per-draw call basis
    glEnable(GL_CULL_FACE);

    // Query available display modes (resolution, bit depth per channel and
    // refresh rates)
    I32 numberOfDisplayModes = 0;
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
                tempDisplayMode._refreshRate.push_back(static_cast<U8>(displayMode.refresh_rate));
                Util::ReplaceStringInPlace(tempDisplayMode._formatName, "SDL_PIXELFORMAT_", "");
                GFX_DEVICE.gpuState().registerDisplayMode(static_cast<U8>(display), tempDisplayMode);
                tempDisplayMode._refreshRate.clear();
            }
        }
    }

    // Prepare font rendering subsystem
    if (!createFonsContext()) {
        Console::errorfn(Locale::get("ERROR_FONT_INIT"));
        return ErrorCode::FONT_INIT_ERROR;
    }

    // Prepare immediate mode emulation rendering
    NS_GLIM::glim.SetVertexAttribLocation(to_uint(AttribLocation::VERTEX_POSITION));
    // We need a dummy VAO object for point rendering
    GLUtil::DSAWrapper::dsaCreateVertexArrays(1, &_pointDummyVAO);
    // Allocate a buffer for indirect draw used to store the query results
    // without a round-trip to the CPU
    GLUtil::DSAWrapper::dsaCreateBuffers(1, &_indirectDrawBuffer);
    GLUtil::DSAWrapper::dsaNamedBufferData(
        _indirectDrawBuffer,
        Config::MAX_VISIBLE_NODES * sizeof(IndirectDrawCommand), NULL,
        GL_DYNAMIC_DRAW);
    // In debug, we also have various performance counters to profile GPU rendering
    // operations
#ifdef _DEBUG
    // We have multiple counter buffers, and each can be multi-buffered
    // (currently, only double-buffered, front and back)
    // to avoid pipeline stalls
    for (U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i) {
        glGenQueries(PERFORMANCE_COUNTERS, _queryID[i]);
        DIVIDE_ASSERT(
            _queryID[i][0] != 0,
            "GLFWWrapper error: Invalid performance counter query ID!");
    }

    _queryBackBuffer = 4;
    // Initialize an initial time query as it solves certain issues with
    // consecutive queries later
    glBeginQuery(GL_TIME_ELAPSED, _queryID[0][0]);
    glEndQuery(GL_TIME_ELAPSED);
    // Wait until the results are available
    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) {
        glGetQueryObjectiv(_queryID[0][0],
                           GL_QUERY_RESULT_AVAILABLE,
                           &stopTimerAvailable);
    }
#endif

    // Once OpenGL is ready for rendering, init CEGUI
    _enableCEGUIRendering = !(ParamHandler::getInstance().getParam<bool>("GUI.CEGUI.SkipRendering"));
    _GUIGLrenderer = &CEGUI::OpenGL3Renderer::create();
    _GUIGLrenderer->enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    CEGUI::System::create(*_GUIGLrenderer);

    // That's it. Everything should be ready for draw calls
    Console::printfn(Locale::get("START_OGL_API_OK"));

    return ErrorCode::NO_ERR;
}

/// Clear everything that was setup in initRenderingAPI()
void GL_API::closeRenderingAPI() {
    CEGUI::OpenGL3Renderer::destroy(*_GUIGLrenderer);
    _GUIGLrenderer = nullptr;
    // Destroy sampler objects
    MemoryManager::DELETE_HASHMAP(_samplerMap);

    // Destroy the text rendering system
    deleteFonsContext();
    _fonts.clear();
    // If we have an indirect draw buffer, delete it
    if (_indirectDrawBuffer > 0) {
        glDeleteBuffers(1, &_indirectDrawBuffer);
        _indirectDrawBuffer = 0;
    }
    if (_pointDummyVAO > 0) {
        glDeleteVertexArrays(1, &_pointDummyVAO);
        _pointDummyVAO = 0;
    }
    // Destroy the OpenGL Context(s)
    destroyGLContext();
    // Destroy application windows and close SDL
    destroyWindow();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void GL_API::handleChangeWindowType(WindowType newWindowType) {
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    WindowType crtWindowType = _crtWindowType;

    _crtWindowType = newWindowType;
    I32 switchState = 0;
    switch (newWindowType) {
        case WindowType::SPLASH: {
            switchState = SDL_SetWindowFullscreen(GLUtil::_mainWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(GLUtil::_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(GLUtil::_mainWindow, SDL_FALSE);
        } break;
        case WindowType::WINDOW: {
            switchState = SDL_SetWindowFullscreen(GLUtil::_mainWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(GLUtil::_mainWindow, SDL_TRUE);
            SDL_SetWindowGrab(GLUtil::_mainWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN_WINDOWED: {
            switchState = SDL_SetWindowFullscreen(GLUtil::_mainWindow, 0);
            assert(switchState >= 0);

            SDL_SetWindowBordered(GLUtil::_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(GLUtil::_mainWindow, SDL_FALSE);
        } break;
        case WindowType::FULLSCREEN: {
            switchState = SDL_SetWindowFullscreen(GLUtil::_mainWindow, SDL_WINDOW_FULLSCREEN);
            assert(switchState >= 0);

            SDL_SetWindowBordered(GLUtil::_mainWindow, SDL_FALSE);
            SDL_SetWindowGrab(GLUtil::_mainWindow, SDL_TRUE);
        } break;
    };

    const vec2<U16>& dimensions = winManager.getWindowDimensions(newWindowType);
    changeWindowSize(dimensions.width, dimensions.height);

    centerWindowPosition();

    if (crtWindowType == WindowType::COUNT) {
        SDL_ShowWindow(GLUtil::_mainWindow);
    }

}

void GL_API::changeWindowSize(U16 w, U16 h) {

    WindowManager& winManager = Application::getInstance().getWindowManager();
    WindowType type = winManager.mainWindowType();

    if (_externalResizeEvent && type != WindowType::WINDOW) {
        return;
    }

    if (type == WindowType::FULLSCREEN) {
        // Should never be true, but need to be sure
        SDL_DisplayMode mode, closestMode;
        SDL_GetCurrentDisplayMode(winManager.targetDisplay(), &mode);
        mode.w = w;
        mode.h = h;
        SDL_GetClosestDisplayMode(winManager.targetDisplay(), &mode, &closestMode);
        SDL_SetWindowDisplayMode(GLUtil::_mainWindow, &closestMode);
    } else {
        if (_externalResizeEvent) {
            // Find a decent resolution close to our dragged dimensions
            SDL_DisplayMode mode, closestMode;
            SDL_GetCurrentDisplayMode(winManager.targetDisplay(), &mode);
            mode.w = w;
            mode.h = h;
            SDL_GetClosestDisplayMode(winManager.targetDisplay(), &mode, &closestMode);
            w = static_cast<U16>(closestMode.w);
            h = static_cast<U16>(closestMode.h);
        }

        SDL_SetWindowSize(GLUtil::_mainWindow, w, h);
        if (winManager.getWindowDimensions() != vec2<U16>(w, h)) {
            winManager.setWindowDimensions(winManager.mainWindowType(), 
                                           vec2<U16>(w, h));
        }
    }
}

void GL_API::changeResolution(GLushort w, GLushort h) {
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    if (winManager.mainWindowType() == WindowType::WINDOW) {
        changeWindowSize(w, h);
    }
}

/// Window positioning is handled by SDL
void GL_API::setWindowPosition(U16 w, U16 h) {
    _internalMoveEvent = true;
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    if (winManager.getWindowPosition() != vec2<U16>(w, h)) {
        SDL_SetWindowPosition(GLUtil::_mainWindow, w, h);
    }
}

/// Centering is also easier via SDL
void GL_API::centerWindowPosition() {
    _internalMoveEvent = true;
    const WindowManager& winManager = Application::getInstance().getWindowManager();
    SDL_SetWindowPosition(GLUtil::_mainWindow,
            SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay()),
            SDL_WINDOWPOS_CENTERED_DISPLAY(winManager.targetDisplay()));
}

/// Mouse positioning is handled by SDL
void GL_API::setCursorPosition(GLushort x, GLushort y) {
    SDL_WarpMouseInWindow(GLUtil::_mainWindow, x, y);
}

/// This functions should be run in a separate, consumer thread.
/// The main app thread, the producer, adds tasks via a lock-free queue
void GL_API::threadedLoadCallback() {
    // We need a valid OpenGL context to make current in this thread
    assert(GLUtil::_mainWindow != nullptr);
    SDL_GL_MakeCurrent(GLUtil::_mainWindow, GLUtil::_glLoadingContext);
    glbinding::Binding::initialize();

// Enable OpenGL debug callbacks for this context as well
#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // Debug callback in a separate thread requires a flag to distinguish it
    // from the main thread's callbacks
    glDebugMessageCallback(GLUtil::DebugCallback, (void*)(GLUtil::_glLoadingContext));
#endif
    // Delay startup of the thread by a 1/4 of a second. No real reason
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

};
