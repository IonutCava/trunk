#include "Headers/GLWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#endif //CEGUI_STATIC

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

#include <GLIM/glim.h>
#include <chrono>
#include <thread>

#define HAVE_M_PI
#include <SDL.h>

namespace Divide {

ErrorCode GL_API::createGLContext(const DisplayWindow& window) {
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    GLUtil::_glSecondaryContexts.push_back(SDL_GL_CreateContext(window.getRawWindow()));
    GLUtil::_glRenderContext = SDL_GL_CreateContext(window.getRawWindow());
    if (GLUtil::_glRenderContext == nullptr)
    {
    	Console::errorfn(Locale::get(_ID("ERROR_GFX_DEVICE")),
    					 Locale::get(_ID("ERROR_GL_OLD_VERSION")));
    	Console::printfn(Locale::get(_ID("WARN_SWITCH_D3D")));
    	Console::printfn(Locale::get(_ID("WARN_APPLICATION_CLOSE")));
   		return ErrorCode::OGL_OLD_HARDWARE;
    }

    return ErrorCode::NO_ERR;
}

ErrorCode GL_API::destroyGLContext() {
    SDL_GL_DeleteContext(GLUtil::_glRenderContext);

    for (SDL_GLContext& ctx : GLUtil::_glSecondaryContexts) {
        SDL_GL_DeleteContext(ctx);
    }
    GLUtil::_glSecondaryContexts.clear();

    return ErrorCode::NO_ERR;
}


/// Try and create a valid OpenGL context taking in account the specified
/// resolution and command line arguments
ErrorCode GL_API::initRenderingAPI(GLint argc, char** argv) {
    ParamHandler& par = ParamHandler::getInstance();

    // Fill our (abstract API <-> openGL) enum translation tables with proper
    // values
    GLUtil::fillEnumTables();

    const DisplayWindow& window = Application::getInstance().getWindowManager().getActiveWindow();
    ErrorCode errorState = createGLContext(window);
    if (errorState != ErrorCode::NO_ERR) {
        return errorState;
    }

    SDL_GL_MakeCurrent(window.getRawWindow(), GLUtil::_glRenderContext);
    glbinding::Binding::initialize();
    glbinding::Binding::useCurrentContext();

    // OpenGL has a nifty error callback system, available in every build
    // configuration if required
#if defined(ENABLE_GPU_VALIDATION)
    // GL_DEBUG_OUTPUT_SYNCHRONOUS is essential for debugging gl commands in the IDE
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // hardwire our debug callback function with OpenGL's implementation
    glDebugMessageCallback(GLUtil::DebugCallback, nullptr);
    // nVidia flushes a lot of useful info about buffer allocations and shader
    // recompiles due to state and what now, but those aren't needed until that's
    // what's actually causing the bottlenecks
    /*U32 nvidiaBufferErrors[] = { 131185, 131218 };
    // Disable shader compiler errors (shader class handles that)
    glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER, GL_DEBUG_TYPE_ERROR,
        GL_DONT_CARE, 0, nullptr, GL_FALSE);
    // Disable nVidia buffer allocation info (an easy enable is to change the
    // count param to 0)
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER,
        GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);
    // Shader will be recompiled nVidia error
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE,
        GL_DONT_CARE, 2, nvidiaBufferErrors, GL_FALSE);*/
#endif
    // Vsync is toggled on or off via the external config file
    SDL_GL_SetSwapInterval(par.getParam<bool>(_ID("runtime.enableVSync"), false) ? 1 : 0);
        
    // If we got here, let's figure out what capabilities we have available
    // Maximum addressable texture image units in the fragment shader
    _maxTextureUnits = GLUtil::getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS);

    Console::printfn(Locale::get(_ID("GL_MAX_TEX_UNITS_FRAG")), _maxTextureUnits);
    
    par.setParam<I32>(_ID("rendering.maxTextureSlots"), _maxTextureUnits);
    // Maximum number of color attachments per framebuffer
    par.setParam<I32>(_ID("rendering.maxRenderTargetOutputs"),
                      GLUtil::getIntegerv(GL_MAX_COLOR_ATTACHMENTS));
    // Query GPU vendor to enable/disable vendor specific features
    const char* gpuVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (gpuVendor != nullptr) {
        if (strstr(gpuVendor, "Intel") != nullptr) {
            GFX_DEVICE.setGPUVendor(GPUVendor::INTEL);
        } else if (strstr(gpuVendor, "NVIDIA") != nullptr) {
            GFX_DEVICE.setGPUVendor(GPUVendor::NVIDIA);
        } else if (strstr(gpuVendor, "ATI") != nullptr ||
                   strstr(gpuVendor, "AMD") != nullptr) {
            GFX_DEVICE.setGPUVendor(GPUVendor::AMD);
        }
    } else {
        gpuVendor = "Unknown GPU Vendor";
        GFX_DEVICE.setGPUVendor(GPUVendor::OTHER);
    }

    // Cap max anisotropic level to what the hardware supports
    par.setParam(
        _ID("rendering.anisotropicFilteringLevel"),
        std::min(
            GLUtil::getIntegerv(gl::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT),
            par.getParam<GLint>(_ID("rendering.anisotropicFilteringLevel"), 1)));

    Console::printfn(Locale::get(_ID("GL_MAX_VERSION")),
                     GLUtil::getIntegerv(GL_MAJOR_VERSION),
                     GLUtil::getIntegerv(GL_MINOR_VERSION));

    // Number of sample buffers associated with the framebuffer & MSAA sample
    // count
    GLint samplerBuffers = GLUtil::getIntegerv(GL_SAMPLES);
    GLint sampleCount = GLUtil::getIntegerv(GL_SAMPLE_BUFFERS);
    Console::printfn(Locale::get(_ID("GL_MULTI_SAMPLE_INFO")), sampleCount,
                     samplerBuffers);
    // If we do not support MSAA on a hardware level for whatever reason,
    // override user set MSAA levels
    I32 msaaSamples = par.getParam<I32>(_ID("rendering.MSAAsampless"), 0);
    if (samplerBuffers == 0 || sampleCount == 0) {
        msaaSamples = 0;
    }
    // Print all of the OpenGL functionality info to the console and log
    // How many uniforms can we send to fragment shaders
    Console::printfn(Locale::get(_ID("GL_MAX_UNIFORM")),
                     GLUtil::getIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS));
    // How many uniforms can we send to vertex shaders
    Console::printfn(Locale::get(_ID("GL_MAX_VERT_UNIFORM")),
                     GLUtil::getIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS));
    // How many attributes can we send to a vertex shader
    Console::printfn(Locale::get(_ID("GL_MAX_VERT_ATTRIB")),
                     GLUtil::getIntegerv(GL_MAX_VERTEX_ATTRIBS));
    // Maximum number of texture units we can address in shaders
    Console::printfn(Locale::get(_ID("GL_MAX_TEX_UNITS")),
                     GLUtil::getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
                     par.getParam<I32>(_ID("rendering.maxTextureSlots"), 16));
    // Query shading language version support
    Console::printfn(Locale::get(_ID("GL_GLSL_SUPPORT")),
                     glGetString(GL_SHADING_LANGUAGE_VERSION));
    // GPU info, including vendor, gpu and driver
    Console::printfn(Locale::get(_ID("GL_VENDOR_STRING")), gpuVendor,
                     glGetString(GL_RENDERER), glGetString(GL_VERSION));
    // In order: Maximum number of uniform buffer binding points,
    //           maximum size in basic machine units of a uniform block and
    //           minimum required alignment for uniform buffer sizes and offset
    GLint uboffset = GLUtil::getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    GLint uboSize = GLUtil::getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE);

    Console::printfn(Locale::get(_ID("GL_UBO_INFO")),
                     GLUtil::getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS),
                     uboSize / 1024,
                     uboffset);
    par.setParam<I32>(_ID("rendering.UBOAligment"), uboffset);
    par.setParam<U32>(_ID("rendering.UBOSize"), to_uint(uboSize));
    // In order: Maximum number of shader storage buffer binding points,
    //           maximum size in basic machine units of a shader storage block,
    //           maximum total number of active shader storage blocks that may
    //           be accessed by all active shaders and
    //           minimum required alignment for shader storage buffer sizes and
    //           offset.
    GLint sboffset = GLUtil::getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    Console::printfn(
        Locale::get(_ID("GL_SSBO_INFO")),
        GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
        (GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE) / 1024) / 1024,
        GLUtil::getIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
        sboffset);
    par.setParam<I32>(_ID("rendering.SSBOAligment"), sboffset);
    // Maximum number of subroutines and maximum number of subroutine uniform
    // locations usable in a shader
    Console::printfn(Locale::get(_ID("GL_SUBROUTINE_INFO")),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINES),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS));

    // Seamless cubemaps are a nice feature to have enabled (core since 3.2)
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    //glEnable(GL_FRAMEBUFFER_SRGB);
    // Enable multisampling if we actually support and request it
    msaaSamples  > 0 ? glEnable(GL_MULTISAMPLE) :  glDisable(GL_MULTISAMPLE);

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

    // Prepare font rendering subsystem
    if (!createFonsContext()) {
        Console::errorfn(Locale::get(_ID("ERROR_FONT_INIT")));
        return ErrorCode::FONT_INIT_ERROR;
    }

    // Prepare immediate mode emulation rendering
    NS_GLIM::glim.SetVertexAttribLocation(to_const_uint(AttribLocation::VERTEX_POSITION));
    // We need a dummy VAO object for point rendering
    glCreateVertexArrays(1, &_dummyVAO);

    // In debug, we also have various performance counters to profile GPU rendering
    // operations
#ifdef _DEBUG
    // We have multiple counter buffers, and each can be multi-buffered
    // (currently, only double-buffered, front and back)
    // to avoid pipeline stalls
    for (U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i) {
        for (U8 j = 0; j < PERFORMANCE_COUNTERS; ++j) {
            _queryID[i][j].create();
            DIVIDE_ASSERT(_queryID[i][j].getID() != 0,
                "GLFWWrapper error: Invalid performance counter query ID!");
            // Initialize an initial time query as it solves certain issues with
            // consecutive queries later
            glBeginQuery(GL_TIME_ELAPSED, _queryID[i][j].getID());
            glEndQuery(GL_TIME_ELAPSED);
            // Wait until the results are available
            GLint stopTimerAvailable = 0;
            while (!stopTimerAvailable) {
                glGetQueryObjectiv(_queryID[i][j].getID(), GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
            }
        }
    }

    _queryBackBuffer = PERFORMANCE_COUNTER_BUFFERS - 1;
#endif
    
    // Once OpenGL is ready for rendering, init CEGUI
    _GUIGLrenderer = &CEGUI::OpenGL3Renderer::create();
    _GUIGLrenderer->enableExtraStateSettings(par.getParam<bool>(_ID("GUI.CEGUI.ExtraStates")));
    CEGUI::System::create(*_GUIGLrenderer);

    static const vec4<F32> clearColor = DefaultColors::DIVIDE_BLUE();
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

    // That's it. Everything should be ready for draw calls
    Console::printfn(Locale::get(_ID("START_OGL_API_OK")));

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
    if (_dummyVAO > 0) {
        glDeleteVertexArrays(1, &_dummyVAO);
        _dummyVAO = 0;
    }
    glVertexArray::cleanup();
    GLUtil::clearVBOs();
    destroyGLContext();
}

/// This functions should be run in a separate, consumer thread.
/// The main app thread, the producer, adds tasks via a lock-free queue
void GL_API::threadedLoadCallback() {
    glbinding::ContextHandle glCtx = glbinding::getCurrentContext();
    if (glCtx == 0) {
        const DisplayWindow& window = Application::getInstance().getWindowManager().getActiveWindow();
        SDL_GL_MakeCurrent(window.getRawWindow(), GLUtil::_glSecondaryContexts.front());
        glbinding::Binding::initialize(false);
    // Enable OpenGL debug callbacks for this context as well
    #if defined(ENABLE_GPU_VALIDATION)
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // Debug callback in a separate thread requires a flag to distinguish it
        // from the main thread's callbacks
        glDebugMessageCallback(GLUtil::DebugCallback, GLUtil::_glSecondaryContexts.front());
    #endif
    } else {
        glbinding::Binding::useCurrentContext();
    }
}

};
