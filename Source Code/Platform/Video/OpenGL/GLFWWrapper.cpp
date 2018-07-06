#include "Headers/GLWrapper.h"

#include "GUI/Headers/GUI.h"
#include "core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include <glim.h>

#include <CEGUI/CEGUI.h>

#include <chrono>
#include <thread>

#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

namespace Divide {

/// Try and create a valid OpenGL context taking in account the specified
/// resolution and command line arguments
ErrorCode GL_API::initRenderingAPI(const vec2<GLushort>& resolution, GLint argc,
                                   char** argv) {
    // Fill our (abstract API <-> openGL) enum translation tables with proper
    // values
    GLUtil::fillEnumTables();
    // Most runtime variables are stored in the ParamHandler, including
    // initialization settings retrieved from XML
    ParamHandler& par = ParamHandler::getInstance();
    // Setup error callback function before window creation  to make sure any
    // GLFW init errors are handled.
    glfwSetErrorCallback(GLUtil::glfw_error_callback);
    // Attempt to init GLFW
    if (!glfwInit()) {
        return ErrorCode::GLFW_INIT_ERROR;
    }

#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    // OpenGL error handling is available in any build configurations if the
    // proper defines are in place.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
// Debug context also ensures more robust GLFW stress testing
#if defined(_DEBUG)
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
#endif

#endif
    // Toggle multi-sampling if requested. This options requires a
    // client-restart, sadly.
    glfwWindowHint(GLFW_SAMPLES, GFX_DEVICE.gpuState().MSAAEnabled()
                                     ? GFX_DEVICE.gpuState().MSAASamples()
                                     : 0);

    // I REALLY think re-sizable windows are a bad idea. Increase the resolution
    // instead of messing up render targets
    glfwWindowHint(GLFW_RESIZABLE,
                   par.getParam<bool>("runtime.allowWindowResize", false));
    // 32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    // OpenGL ES is not yet supported, but when added, it will need to mirror
    // OpenGL functionality 1-to-1
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::OpenGLES) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    } else {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef GL_VERSION_4_5
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#endif
    }

    // Open an OpenGL window; resolution and windowed mode is specified in the
    // external XML files
    GLUtil::_mainWindow =
        glfwCreateWindow(resolution.width, resolution.height,
                         par.getParam<stringImpl>("appTitle", "Divide").c_str(),
                         par.getParam<bool>("runtime.windowedMode", true)
                             ? nullptr
                             : glfwGetPrimaryMonitor(),
                         nullptr);
    // Check if we have a valid window
    if (!GLUtil::_mainWindow) {
        glfwTerminate();
        Console::errorfn(Locale::get("ERROR_GFX_DEVICE"),
                         Locale::get("ERROR_GL_OLD_VERSION"));
        Console::printfn(Locale::get("WARN_SWITCH_D3D"));
        Console::printfn(Locale::get("WARN_APPLICATION_CLOSE"));
        return ErrorCode::GLFW_WINDOW_INIT_ERROR;
    }

    getWindowHandle(GLUtil::_mainWindow, Application::getInstance().getSysInfo());

    // The application window will hold the main rendering context
    glfwMakeContextCurrent(GLUtil::_mainWindow);
    // Init OpenGL Bidnings for main context
    glbinding::Binding::initialize(false);
    // Bind the window close request received from GLFW with our custom callback
    glfwSetWindowCloseCallback(GLUtil::_mainWindow,
                               GLUtil::glfw_close_callback);
    // Bind our focus change callback to GLFW's internal wiring
    glfwSetWindowFocusCallback(GLUtil::_mainWindow,
                               GLUtil::glfw_focus_callback);

    // We also create a loader thread in the background with its own GL context.
    // To do this with GLFW, we'll create a second, invisible, window
    glfwWindowHint(GLFW_VISIBLE, 0);
    // The loader window will share context lists with the main window
    GLUtil::_loaderWindow = glfwCreateWindow(1, 1, "divide-res-loader", nullptr,
                                             GLUtil::_mainWindow);
    // The loader window is essential to the engine structure, so fail if we
    // can't create it
    if (!GLUtil::_loaderWindow) {
        glfwTerminate();
        return ErrorCode::GLFW_WINDOW_INIT_ERROR;
    }

    // Get the current display mode used by the focused monitor
    const GLFWvidmode* return_struct =
        glfwGetVideoMode(glfwGetPrimaryMonitor());
    // Attempt to position the window in the center of the screen. Useful for
    // the splash screen
    glfwSetWindowPos(GLUtil::_mainWindow,
                     (return_struct->width - resolution.width) * 0.5f,
                     (return_struct->height - resolution.height) * 0.5f);

    DIVIDE_ASSERT(glfwExtensionSupported("GL_EXT_direct_state_access") == 1,
        "GL_API::initRenderingAPI error: DSA is not supported!");
// OpenGL has a nifty error callback system, available in every build
// configuration if required
#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    // GL_DEBUG_OUTPUT_SYNCHRONOUS is essential for debugging gl commands in the
    // IDE
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // hardwire our debug callback function with OpenGL's implementation
    glDebugMessageCallback(GLUtil::DebugCallback, nullptr);
    // nVidia flushes a lot of useful info about buffer allocations and shader
    // recompiles due to state and what now,
    // but those aren't needed until that's what's actually causing the
    // bottlenecks
    U32 nvidiaBufferErrors[] = {131185, 131218};
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

    // If we got here, let's figure out what capabilities we have available
    // Maximum addressable texture image units in the fragment shader
    _maxTextureUnits = GLUtil::getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS);
    par.setParam<I32>("rendering.maxTextureSlots", _maxTextureUnits);
    // Maximum number of color attachments per framebuffer
    par.setParam<I32>("rendering.maxRenderTargetOutputs",
                      GLUtil::getIntegerv(GL_MAX_COLOR_ATTACHMENTS));
    // Query GPU vendor to enable/disable vendor specific features
    stringImpl gpuVendorByte(
        reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
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
    U8 msaaSamples = par.getParam<I32>("rendering.MSAAsampless", 0);
    if (samplerBuffers == 0 || sampleCount == 0) {
        msaaSamples = 0;
    }
    GFX_DEVICE.gpuState().initAA(par.getParam<I32>("rendering.FXAAsamples", 0),
                                 msaaSamples);
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

    // Set the clear color to the blue color used since the initial OBJ loader
    // days
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    // Seamless cubemaps are a nice feature to have enabled (core since 3.2)
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // Enable multisampling if we actually support and request it
    if (GFX_DEVICE.gpuState().MSAAEnabled()) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

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
    // Vsync is toggled on or off via the external config file
    glfwSwapInterval(par.getParam<bool>("runtime.enableVSync", false) ? 1 : 0);

    // Query available display modes (resolution, bit depth per channel and
    // refresh rates)
    GLint numberOfDisplayModes;
    const GLFWvidmode* modes =
        glfwGetVideoModes(glfwGetPrimaryMonitor(), &numberOfDisplayModes);
    DIVIDE_ASSERT(
        modes != nullptr,
        "GLFWWrapper error: No display modes found for the current monitor!");
    Console::printfn(Locale::get("AVAILABLE_VIDEO_MODES"),
                     numberOfDisplayModes);
    // Register the display modes with the GFXDevice object
    GPUState::GPUVideoMode tempDisplayMode;
    for (U16 i = 0; i < numberOfDisplayModes; ++i) {
        const GLFWvidmode& temp = modes[i];
        tempDisplayMode._resolution.set(temp.width, temp.height);
        tempDisplayMode._bitDepth.set(temp.redBits, temp.greenBits,
                                      temp.blueBits);
        tempDisplayMode._refreshRate.push_back(temp.refreshRate);
        GFX_DEVICE.gpuState().registerDisplayMode(tempDisplayMode);
        tempDisplayMode._refreshRate.clear();
    }

    stringImpl refreshRates;
    const vectorImpl<GPUState::GPUVideoMode>& registeredModes = GFX_DEVICE.gpuState().getDisplayModes();
    for (const GPUState::GPUVideoMode& mode : registeredModes) {
        // Optionally, output to console/file each display mode
        refreshRates = std::to_string(mode._refreshRate.front());
        vectorAlg::vecSize refreshRateCount = mode._refreshRate.size();
        for (vectorAlg::vecSize i = 1; i < refreshRateCount; ++i) {
            refreshRates += ", " + std::to_string(mode._refreshRate[i]);
        }
        Console::d_printfn(Locale::get("CURRENT_DISPLAY_MODE"), mode._resolution.width,
            mode._resolution.height, mode._bitDepth.r, mode._bitDepth.g,
            mode._bitDepth.b, refreshRates.c_str());
    }

    // Prepare font rendering subsystem
    createFonsContext();
    if (_fonsContext == nullptr) {
        Console::errorfn(Locale::get("ERROR_FONT_INIT"));
        return ErrorCode::GLFW_WINDOW_INIT_ERROR;
    }

    // Prepare immediate mode emulation rendering
    NS_GLIM::glim.SetVertexAttribLocation(
        to_uint(AttribLocation::VERTEX_POSITION));

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
        glGetQueryObjectiv(_queryID[0][0], GL_QUERY_RESULT_AVAILABLE,
                           &stopTimerAvailable);
    }
#endif

    // That's it. Everything should be ready for draw calls
    Console::printfn(Locale::get("START_OGL_API_OK"));

    // Once OpenGL is ready for rendering, init CEGUI
    _enableCEGUIRendering = !(ParamHandler::getInstance().getParam<bool>(
        "GUI.CEGUI.SkipRendering"));
    _GUIGLrenderer = &CEGUI::OpenGL3Renderer::create();
    _GUIGLrenderer->enableExtraStateSettings(
        par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    CEGUI::System::create(*_GUIGLrenderer);

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
    // Destroy application windows and close GLFW
    glfwDestroyWindow(GLUtil::_loaderWindow);
    glfwDestroyWindow(GLUtil::_mainWindow);
    glfwTerminate();
}

/// changeResolution is simply asking GLFW to do the resizing and updating the
/// resolution cache
void GL_API::changeResolution(GLushort w, GLushort h) {
    glfwSetWindowSize(GLUtil::_mainWindow, w, h);
    _cachedResolution.set(w, h);
}

/// Window positioning is handled by GLFW
void GL_API::setWindowPos(GLushort w, GLushort h) const {
    glfwSetWindowPos(GLUtil::_mainWindow, w, h);
}

/// Mouse positioning is handled by GLFW
void GL_API::setCursorPosition(GLushort x, GLushort y) const {
    glfwSetCursorPos(GLUtil::_mainWindow, (GLdouble)x, (GLdouble)y);
}

/// This functions should be run in a separate, consumer thread.
/// The main app thread, the producer, adds tasks via a lock-free queue
void GL_API::threadedLoadCallback() {
    // We need a valid OpenGL context to make current in this thread
    assert(GLUtil::_loaderWindow != nullptr);
    glfwMakeContextCurrent(GLUtil::_loaderWindow);
    glbinding::Binding::initialize();

// Enable OpenGL debug callbacks for this context as well
#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // Debug callback in a separate thread requires a flag to distinguish it
    // from the main thread's callbacks
    glDebugMessageCallback(GLUtil::DebugCallback, (void*)(GLUtil::_loaderWindow));
#endif
    // Delay startup of the thread by a 1/4 of a second. No real reason
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

};