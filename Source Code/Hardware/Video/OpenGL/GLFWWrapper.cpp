#include "Headers/GLWrapper.h"

#include "GUI/Headers/GUI.h"
#include "core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include <glim.h>

#include <CEGUI/CEGUI.h>

/// Try and create a valid OpenGL context taking in account the specified resolution and command line arguments
ErrorCode GL_API::initRenderingApi(const vec2<GLushort>& resolution, GLint argc, char **argv) {
    // Fill our (abstract API <-> openGL) enum translation tables with proper values
    Divide::GLUtil::GL_ENUM_TABLE::fill();
    // Most runtime variables are stored in the ParamHandler, including initialization settings retrieved from XML
    ParamHandler& par = ParamHandler::getInstance();
    // Setup error callback function before window creation  to make sure any GLFW init errors are handled.
    glfwSetErrorCallback(Divide::GLUtil::glfw_error_callback);
    // Attempt to init GLFW
    if (!glfwInit()) {
    	return GLFW_INIT_ERROR;
    }

#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    // OpenGL error handling is available in any build configurations if the proper defines are in place.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
    // Debug context also ensures more robust GLFW stress testing
#if defined(_DEBUG)
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
#endif

#endif
    // Toggle multi-sampling if requested. This options requires a client-restart, sadly.    
    if (GFX_DEVICE.MSAAEnabled()) {
        glfwWindowHint(GLFW_SAMPLES, GFX_DEVICE.MSAASamples());
    }

    // OpenGL ES is not yet supported, but when added, it will need to mirror OpenGL functionality 1-to-1
    if (getId() == OpenGLES) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    } else {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, Config::Profile::DISABLE_PERSISTENT_BUFFER ? 3 : 4);
    }

    // I REALLY think re-sizable windows are a bad idea. Increase the resolution instead of messing up render targets
    glfwWindowHint(GLFW_RESIZABLE, par.getParam<bool>("runtime.allowWindowResize",false));
    // 32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
    glfwWindowHint(GLFW_RED_BITS,     8);
    glfwWindowHint(GLFW_GREEN_BITS,   8);
    glfwWindowHint(GLFW_BLUE_BITS,    8);
    glfwWindowHint(GLFW_ALPHA_BITS,   8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS,  24);
    
    // Open an OpenGL window; resolution and windowed mode is specified in the external XML files
    Divide::GLUtil::_mainWindow = glfwCreateWindow(resolution.width, resolution.height, par.getParam<std::string>("appTitle").c_str(),
                                                   par.getParam<bool>("runtime.windowedMode", true) ? nullptr : glfwGetPrimaryMonitor(), nullptr);
    // Check if we have a valid window
    if (!Divide::GLUtil::_mainWindow) {
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

    // The application window will hold the main rendering context
    glfwMakeContextCurrent(Divide::GLUtil::_mainWindow);
    // Init glew for main context
    Divide::GLUtil::initGlew();
    // Bind the window close request received from GLFW with our custom callback
    glfwSetWindowCloseCallback(Divide::GLUtil::_mainWindow, Divide::GLUtil::glfw_close_callback);
    // Bind our focus change callback to GLFW's internal wiring
    glfwSetWindowFocusCallback(Divide::GLUtil::_mainWindow, Divide::GLUtil::glfw_focus_callback);
    // Geometry shaders became core in version 3.3, shader storage buffers in 4.3, buffer storage in 4.4 so fail if we are missing the required version
    if ((Config::Profile::DISABLE_PERSISTENT_BUFFER && !GLEW_VERSION_4_3) || (!Config::Profile::DISABLE_PERSISTENT_BUFFER && !GLEW_VERSION_4_4)) {
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE"), Locale::get("ERROR_GL_OLD_VERSION"));
        return GLEW_OLD_HARDWARE;
    }

    // We also create a loader thread in the background with its own GL context. To do this with GLFW, we'll create a second, invisible, window
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    // The loader window will share context lists with the main window
    Divide::GLUtil::_loaderWindow = glfwCreateWindow(1, 1, "divide-res-loader", nullptr, Divide::GLUtil::_mainWindow);
    // The loader window is essential to the engine structure, so fail if we can't create it
    if (!Divide::GLUtil::_loaderWindow) {
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

    // Get the current display mode used by the focused monitor
    const GLFWvidmode* return_struct = glfwGetVideoMode(glfwGetPrimaryMonitor());
    // Attempt to position the window in the center of the screen. Useful for the splash screen
    glfwSetWindowPos(Divide::GLUtil::_mainWindow, (return_struct->width - resolution.width) * 0.5f, (return_struct->height - resolution.height) * 0.5f);

    // OpenGL has a nifty error callback system, available in every build configuration if required
#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
    // GL_DEBUG_OUTPUT_SYNCHRONOUS is essential for debugging gl commands in the IDE
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // hardwire our debug callback function with OpenGL's implementation
    glDebugMessageCallback(&Divide::GLUtil::DebugCallback, NULL);
    // nVidia flushes a lot of useful info about buffer allocations and shader recompiles due to state and what now,
    // but those aren't needed until that's what's actually causing the bottlenecks
    U32 nvidiaBufferErrors[] = {131185, 131218};
    // Disable shader compiler errors (shader class handles that)
    glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    // Disable nVidia buffer allocation info (an easy enable is to change the count param to 0)
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);
    // Shader will be recompiled nVidia error
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);
#endif

    // If we got here, let's figure out what capabilities we have available
    // Maximum addressable texture image units in the fragment shader
    GFX_DEVICE.setMaxTextureSlots(Divide::GLUtil::getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS));
    // Maximum number of color attachments per framebuffer
    GFX_DEVICE.setMaxRenderTargetOutputs(Divide::GLUtil::getIntegerv(GL_MAX_COLOR_ATTACHMENTS));
    // Query GPU vendor to enable/disable vendor specific features
    std::string gpuVendorByte(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    if (!gpuVendorByte.empty()) {
             if(gpuVendorByte.compare(0,5,"Intel")  == 0) { setGPUVendor(GPU_VENDOR_INTEL);  }
        else if(gpuVendorByte.compare(0,6,"NVIDIA") == 0) { setGPUVendor(GPU_VENDOR_NVIDIA); }
        else if(gpuVendorByte.compare(0,3,"ATI")    == 0) { setGPUVendor(GPU_VENDOR_AMD);    }
        else if(gpuVendorByte.compare(0,3,"AMD")    == 0) { setGPUVendor(GPU_VENDOR_AMD);    }
    } else {
        gpuVendorByte = "Unknown GPU Vendor";
        setGPUVendor(GPU_VENDOR_OTHER);
    }

    // Cap max anisotropic level to what the hardware supports
    par.setParam("rendering.anisotropicFilteringLevel", std::min(Divide::GLUtil::getIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT), 
                                                                 par.getParam<GLint>("rendering.anisotropicFilteringLevel", 1)));
    // OpenGL major version ( we do not support OpenGL versions lower than 4.x )
    if (Divide::GLUtil::getIntegerv(GL_MAJOR_VERSION) < 4) {
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE"), Locale::get("ERROR_GL_OLD_VERSION"));
        return GLEW_OLD_HARDWARE;
    } else {
        PRINT_FN(Locale::get("GL_MAX_VERSION"), 4, GLEW_VERSION_4_4 ? 4 : 3);
    }

    // Number of sample buffers associated with the framebuffer & MSAA sample count
    GLint samplerBuffers = Divide::GLUtil::getIntegerv(GL_SAMPLES);
    GLint sampleCount =  Divide::GLUtil::getIntegerv(GL_SAMPLE_BUFFERS);
    PRINT_FN(Locale::get("GL_MULTI_SAMPLE_INFO"), sampleCount, samplerBuffers);
    // If we do not support MSAA on a hardware level for whatever reason, override user set MSAA levels
    U8 msaaSamples = par.getParam<I32>("rendering.MSAAsampless", 0);
    if (samplerBuffers == 0 || sampleCount == 0) {
        msaaSamples = 0;
    }
    GFX_DEVICE.initAA(par.getParam<I32>("rendering.FXAAsamples", 0), msaaSamples);
    // Print all of the OpenGL functionality info to the console and log
    // How many uniforms can we send to fragment shaders
    PRINT_FN(Locale::get("GL_MAX_UNIFORM"), Divide::GLUtil::getIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS));
    // How many varying floats can we use inside a shader program
    PRINT_FN(Locale::get("GL_MAX_FRAG_VARYING"), Divide::GLUtil::getIntegerv(GL_MAX_VARYING_FLOATS));
    // How many uniforms can we send to vertex shaders
    PRINT_FN(Locale::get("GL_MAX_VERT_UNIFORM"),Divide::GLUtil::getIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS));
    // How many attributes can we send to a vertex shader
    PRINT_FN(Locale::get("GL_MAX_VERT_ATTRIB"),Divide::GLUtil::getIntegerv(GL_MAX_VERTEX_ATTRIBS));
    // Maximum number of texture units we can address in shaders
    PRINT_FN(Locale::get("GL_MAX_TEX_UNITS"), Divide::GLUtil::getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS), GFX_DEVICE.getMaxTextureSlots());
    // Query shading language version support
    PRINT_FN(Locale::get("GL_GLSL_SUPPORT"), glGetString(GL_SHADING_LANGUAGE_VERSION));
    // GPU info, including vendor, gpu and driver
    PRINT_FN(Locale::get("GL_VENDOR_STRING"), gpuVendorByte.c_str(), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    // In order: Maximum number of uniform buffer binding points, 
    //           maximum size in basic machine units of a uniform block and
    //           minimum required alignment for uniform buffer sizes and offset
    PRINT_FN(Locale::get("GL_UBO_INFO"), Divide::GLUtil::getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS), 
                                         Divide::GLUtil::getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE) / 1024, 
                                         Divide::GLUtil::getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));

    // In order: Maximum number of shader storage buffer binding points, maximum size in basic machine units of a shader storage block,
    //           maximum total number of active shader storage blocks that may be accessed by all active shaders and 
    //           minimum required alignment for shader storage buffer sizes and offset.
    PRINT_FN(Locale::get("GL_SSBO_INFO"), Divide::GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
                                         (Divide::GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE) / 1024) / 1024, 
                                          Divide::GLUtil::getIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS), 
                                          Divide::GLUtil::getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));

    // Maximum number of subroutines and maximum number of subroutine uniform locations usable in a shader
    PRINT_FN(Locale::get("GL_SUBROUTINE_INFO"), Divide::GLUtil::getIntegerv(GL_MAX_SUBROUTINES), Divide::GLUtil::getIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS));
        
    // Set the clear color to the blue color used since the initial OBJ loader days
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    // Seamless cubemaps are a nice feature to have enabled (core since 3.2)
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // Enable multisampling if we actually support and request it
    if (GFX_DEVICE.MSAAEnabled()) {
        glEnable(GL_MULTISAMPLE);
    }

    // Line smoothing should almost always be used
    if (Config::USE_HARDWARE_AA_LINES) {
        glEnable(GL_LINE_SMOOTH);
        glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, &_lineWidthLimit);
    } else {
        glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, &_lineWidthLimit);
    }

    // Culling is enabled by default, but RenderStateBlocks can toggle it on a per-draw call basis
    glEnable(GL_CULL_FACE);
    // Primitive restart indexes can either be a predefined short value (_S) or a predefined int value (_L), depending on the index buffer
    glPrimitiveRestartIndex(Config::PRIMITIVE_RESTART_INDEX_S);
    // Vsync is toggled on or off via the external config file
    glfwSwapInterval(par.getParam<bool>("runtime.enableVSync",false) ? 1 : 0);

    // Query available display modes (resolution, bit depth per channel and refresh rates)
    GLint numberOfDisplayModes;
    const GLFWvidmode* modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &numberOfDisplayModes );
    DIVIDE_ASSERT(modes != nullptr, "GLFWWrapper error: No display modes found for the current monitor!");
    PRINT_FN(Locale::get("AVAILABLE_VIDEO_MODES"), numberOfDisplayModes);
    // Register the display modes with the GFXDevice object
    GFXDevice::GPUVideoMode tempDisplayMode;
    for(U16 i = 0; i < numberOfDisplayModes; ++i){
        const GLFWvidmode& temp = modes[i];
        tempDisplayMode._resolution.set(temp.width, temp.height);
        tempDisplayMode._bitDepth.set(temp.redBits, temp.greenBits, temp.blueBits);
        tempDisplayMode._refreshRate =  temp.refreshRate;
        GFX_DEVICE.registerDisplayMode(tempDisplayMode);
        // Optionally, output to console/file each display mode
        D_PRINT_FN(Locale::get("CURRENT_DISPLAY_MODE"), temp.width, temp.height, temp.redBits, temp.greenBits, temp.blueBits, temp.refreshRate);
    }

    // Prepare font rendering subsystem
    createFonsContext();  
    if (_fonsContext == nullptr) {
        ERROR_FN(Locale::get("ERROR_FONT_INIT"));
        return GLFW_WINDOW_INIT_ERROR;
    }

    // Prepare immediate mode emulation rendering
    NS_GLIM::glim.SetVertexAttribLocation(Divide::VERTEX_POSITION_LOCATION);

    // We need a dummy VAO object for point rendering
    glGenVertexArrays(1, &_pointDummyVAO);

    // In debug, we also have various performance counters to profile GPU rendering operations
#   ifdef _DEBUG
        // We have multiple counter buffers, and each can be multi-buffered (currently, only double-buffered, front and back) to avoid pipeline stalls
        for (U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i) {
            glGenQueries(PERFORMANCE_COUNTERS, _queryID[i]);
            DIVIDE_ASSERT(_queryID[i][0] != 0, "GLFWWrapper error: Invalid performance counter query ID!");
        }

        _queryBackBuffer = 4;
        // Initialize an initial time query as it solves certain issues with consecutive queries later
        glBeginQuery(GL_TIME_ELAPSED, _queryID[0][0]);
        glEndQuery(GL_TIME_ELAPSED);
        // Wait until the results are available
        GLint stopTimerAvailable = 0;
        while (!stopTimerAvailable) {
            glGetQueryObjectiv(_queryID[0][0], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
        }
#   endif

    // That's it. Everything should be ready for draw calls
    PRINT_FN(Locale::get("START_OGL_API_OK"));

    // Once OpenGL is ready for rendering, init CEGUI
    _enableCEGUIRendering =  !(ParamHandler::getInstance().getParam<bool>("GUI.CEGUI.SkipRendering"));
    CEGUI::OpenGL3Renderer& GUIGLrenderer = CEGUI::OpenGL3Renderer::create();
    GUIGLrenderer.enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    CEGUI::System::create(GUIGLrenderer);

    return NO_ERR;
}

/// Clear everything that was setup in initRenderingApi()
void GL_API::closeRenderingApi(){
    // Close the loading thread 
    _closeLoadingThread = true;
    while(GFX_DEVICE.loadingThreadAvailable()){
        boost::this_thread::sleep_for(boost::chrono::milliseconds(20));//<Avoid burning the CPU - Ionut
    }

    // Destroy sampler objects
    FOR_EACH(samplerObjectMap::value_type& it, _samplerMap) {
        SAFE_DELETE(it.second);
    }
    _samplerMap.clear();

    // Destroy the text rendering system
    deleteFonsContext();
    _fonts.clear();

    // Destroy application windows and close GLFW
    glfwDestroyWindow(Divide::GLUtil::_loaderWindow);
    glfwDestroyWindow(Divide::GLUtil::_mainWindow);
    glfwTerminate();
}

/// changeResolutionInternal is simply asking GLFW to do the resizing and updating the resolution cache
void GL_API::changeResolutionInternal(GLushort w, GLushort h) {
    glfwSetWindowSize(Divide::GLUtil::_mainWindow, w, h);
    _cachedResolution.set(w, h);
}

/// Window positioning is handled by GLFW
void GL_API::setWindowPos(GLushort w, GLushort h) const {
    glfwSetWindowPos(Divide::GLUtil::_mainWindow, w, h);
}

/// Mouse positioning is handled by GLFW
void GL_API::setCursorPosition(GLushort x, GLushort y) const {
    glfwSetCursorPos(Divide::GLUtil::_mainWindow, (GLdouble)x, (GLdouble)y);
}

/// This functions should be run in a separate, consumer thread.
/// The main app thread, the producer, adds tasks via a lock-free queue that is checked every 20 ms
void GL_API::createLoaderThread() {
    // We need a valid OpenGL context to make current in this thread
    assert(Divide::GLUtil::_loaderWindow != nullptr);
    glfwMakeContextCurrent(Divide::GLUtil::_loaderWindow);

#   ifdef GLEW_MX
        Divide::GLUtil::initGlew();
        // Enable OpenGL debug callbacks for this context as well
#       if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            // Debug callback in a separate thread requires a flag to distinguish it from the main thread's callbacks
            glDebugMessageCallback(&Divide::GLUtil::DebugCallback, (void*)(1));
#      endif
#   endif

    // This will be our target container for new items pulled from the queue
    DELEGATE_CBK callback;

    // Delay startup of the thread by a 1/4 of a second. No real reason
    boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
    // Use an atomic bool to check if the thread is still active (cheap and easy)
    GFX_DEVICE.loadingThreadAvailable(true);
    // Run an infinite loop until we actually request otherwise
    while (!_closeLoadingThread) {
        // Try to pull a new element from the queue
        if (GFX_DEVICE.getLoadQueue().pop(callback)) {
            // If we manage, we run it and force the gpu to process it
            callback();
            glFlush();
        } else {  // If there are no new items to process in the queue, just stall
            // Avoid burning the CPU - Ionut
            boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
        }
    }
    // If we close the loading thread, update our atomic bool to make sure the application isn't using it anymore
    GFX_DEVICE.loadingThreadAvailable(false);
}