#include "stdafx.h"

#include "Headers/GLWrapper.h"
#include "Headers/glHardwareQuery.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

#include <AntTweakBar/include/AntTweakBar.h>

#include <GLIM/glim.h>
#include <chrono>
#include <thread>

#define HAVE_M_PI
#include <SDL.h>

namespace Divide {
namespace {
    const U32 g_maxVAOS = 512u;
    const U32 g_maxQueryRings = 64;

    class ContextPool {
    public:
        ContextPool()
        {
            _contexts.reserve(HARDWARE_THREAD_COUNT() * 2);
        }

        ~ContextPool() 
        {
            assert(_contexts.empty());
        }

        bool init(U32 size, const DisplayWindow& window) {
            SDL_Window* raw = window.getRawWindow();
            WriteLock w_lock(_glContextLock);
            _contexts.resize(size, std::make_pair(nullptr, false));
            for (std::pair<SDL_GLContext, bool>& ctx : _contexts) {
                ctx.first = SDL_GL_CreateContext(raw);
            }
            return true;
        }

        bool destroy() {
            WriteLock w_lock(_glContextLock);
            for (std::pair<SDL_GLContext, bool>& ctx : _contexts) {
                SDL_GL_DeleteContext(ctx.first);
            }
            _contexts.clear();
            return true;
        }

        bool getAvailableContext(SDL_GLContext& ctx) {
            UpgradableReadLock ur_lock(_glContextLock);
            for (std::pair<SDL_GLContext, bool>& ctxIt : _contexts) {
                if (!ctxIt.second) {
                    UpgradeToWriteLock w_lock(ur_lock);
                    ctx = ctxIt.first;
                    ctxIt.second = true;
                    return true;
                }
            }

            return false;
        }

    private:
        SharedLock _glContextLock;
        vectorImpl<std::pair<SDL_GLContext, bool /*in use*/>> _contexts;
    } g_ContextPool;
};

ErrorCode GL_API::createGLContext(const DisplayWindow& window) {
    g_ContextPool.init(HARDWARE_THREAD_COUNT() * 2, window);
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
    g_ContextPool.destroy();

    return ErrorCode::NO_ERR;
}


/// Try and create a valid OpenGL context taking in account the specified resolution and command line arguments
ErrorCode GL_API::initRenderingAPI(GLint argc, char** argv, Configuration& config) {
    // Fill our (abstract API <-> openGL) enum translation tables with proper values
    GLUtil::fillEnumTables();

    const DisplayWindow& window = _context.parent().platformContext().app().windowManager().getActiveWindow();
    ErrorCode errorState = createGLContext(window);
    if (errorState != ErrorCode::NO_ERR) {
        return errorState;
    }

    SDL_GL_MakeCurrent(window.getRawWindow(), GLUtil::_glRenderContext);
    glbinding::Binding::initialize();
    if (glbinding::getCurrentContext() == 0) {
        return ErrorCode::GLBINGING_INIT_ERROR;
    }

    glbinding::Binding::useCurrentContext();

    if (s_hardwareQueryPool == nullptr) {
        s_hardwareQueryPool = MemoryManager_NEW glHardwareQueryPool(_context);
    }

    // OpenGL has a nifty error callback system, available in every build
    // configuration if required
    if (Config::ENABLE_GPU_VALIDATION) {
        // GL_DEBUG_OUTPUT_SYNCHRONOUS is essential for debugging gl commands in the IDE
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // hardwire our debug callback function with OpenGL's implementation
        glDebugMessageCallback((GLDEBUGPROC)GLUtil::DebugCallback, nullptr);
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
    }

    // Vsync is toggled on or off via the external config file
    SDL_GL_SetSwapInterval(config.runtime.enableVSync ? 1 : 0);

    // Query GPU vendor to enable/disable vendor specific features
    GPUVendor vendor = GPUVendor::COUNT;
    const char* gpuVendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (gpuVendorStr != nullptr) {
        if (strstr(gpuVendorStr, "Intel") != nullptr) {
            vendor = GPUVendor::INTEL;
        } else if (strstr(gpuVendorStr, "NVIDIA") != nullptr) {
            vendor = GPUVendor::NVIDIA;
        } else if (strstr(gpuVendorStr, "ATI") != nullptr || strstr(gpuVendorStr, "AMD") != nullptr) {
            vendor = GPUVendor::AMD;
        } else if(strstr(gpuVendorStr, "Microsoft") != nullptr) {
            vendor = GPUVendor::MICROSOFT;
        } else {
            vendor = GPUVendor::OTHER;
        }
    } else {
        gpuVendorStr = "Unknown GPU Vendor";
        vendor = GPUVendor::OTHER;
    }
    GPURenderer renderer = GPURenderer::COUNT;
    const char* gpuRendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (gpuRendererStr != nullptr) {
        if (strstr(gpuRendererStr,"Tegra") || strstr(gpuRendererStr, "GeForce") || strstr(gpuRendererStr, "NV")) {
            renderer = GPURenderer::GEFORCE;
        } else if(strstr(gpuRendererStr, "PowerVR") || strstr(gpuRendererStr, "Apple")) {
            renderer = GPURenderer::POWERVR;
            vendor = GPUVendor::IMAGINATION_TECH;
        } else if(strstr(gpuRendererStr, "Mali")) {
            renderer = GPURenderer::MALI;
            vendor = GPUVendor::ARM;
        } else if (strstr(gpuRendererStr, "Adreno")) {
            renderer = GPURenderer::ADRENO;
            vendor = GPUVendor::QUALCOMM;
        } else if(strstr(gpuRendererStr, "AMD") || strstr(gpuRendererStr, "ATI")) {
            renderer = GPURenderer::RADEON;
        } else if (strstr(gpuRendererStr, "Intel")) {
            renderer = GPURenderer::INTEL;
        } else if(strstr(gpuRendererStr, "Vivante")) {
            renderer = GPURenderer::VIVANTE;
            vendor = GPUVendor::VIVANTE;
        } else if(strstr(gpuRendererStr, "VideoCore")) {
            renderer = GPURenderer::VIDEOCORE;
            vendor = GPUVendor::ALPHAMOSAIC;
        } else if (strstr(gpuRendererStr, "WebKit") || strstr(gpuRendererStr, "Mozilla") || strstr(gpuRendererStr, "ANGLE")) {
            renderer = GPURenderer::WEBGL;
            vendor = GPUVendor::WEBGL;
        } else if (strstr(gpuRendererStr, "GDI Generic")) {
            renderer = GPURenderer::GDI;
        } else {
            renderer = GPURenderer::UNKNOWN;
        }
    } else {
        gpuRendererStr = "Unknown GPU Renderer";
        renderer = GPURenderer::UNKNOWN;
    }

    GFXDevice::setGPURenderer(renderer);
    GFXDevice::setGPUVendor(vendor);
    
    // If we got here, let's figure out what capabilities we have available
    // Maximum addressable texture image units in the fragment shader
    s_maxTextureUnits = std::max(GLUtil::getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS), 8);
    s_maxAttribBindings = GLUtil::getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS);
    s_vaoBufferData.init(s_maxAttribBindings);

    if (to_base(ShaderProgram::TextureUsage::COUNT) >= to_U32(s_maxTextureUnits)) {
        Console::errorfn(Locale::get(_ID("ERROR_INSUFFICIENT_TEXTURE_UNITS")));
        return ErrorCode::GFX_NOT_SUPPORTED;
    }

    if (to_base(AttribLocation::COUNT) >= to_U32(s_maxAttribBindings)) {
        Console::errorfn(Locale::get(_ID("ERROR_INSUFFICIENT_ATTRIB_BINDS")));
        return ErrorCode::GFX_NOT_SUPPORTED;
    }

    Console::printfn(Locale::get(_ID("GL_MAX_TEX_UNITS_FRAG")), s_maxTextureUnits);

    // Maximum number of colour attachments per framebuffer
    s_maxFBOAttachments = GLUtil::getIntegerv(GL_MAX_COLOR_ATTACHMENTS);
    GL_API::s_blendProperties.resize(s_maxFBOAttachments);
    GL_API::s_blendEnabled.resize(s_maxFBOAttachments, GL_FALSE);

    // Cap max anisotropic level to what the hardware supports
    CLAMP(config.rendering.anisotropicFilteringLevel, to_U8(0), to_U8(GLUtil::getIntegerv(gl::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)));
    GL_API::s_anisoLevel = config.rendering.anisotropicFilteringLevel;

    Console::printfn(Locale::get(_ID("GL_MAX_VERSION")),
                     GLUtil::getIntegerv(GL_MAJOR_VERSION),
                     GLUtil::getIntegerv(GL_MINOR_VERSION));

    // Number of sample buffers associated with the framebuffer & MSAA sample
    // count
    GLint samplerBuffers = GLUtil::getIntegerv(GL_SAMPLES);
    GLint sampleCount = GLUtil::getIntegerv(GL_SAMPLE_BUFFERS);
    Console::printfn(Locale::get(_ID("GL_MULTI_SAMPLE_INFO")), sampleCount, samplerBuffers);
    // If we do not support MSAA on a hardware level for whatever reason, override user set MSAA levels
    if (samplerBuffers == 0 || sampleCount == 0) {
        config.rendering.msaaSamples = 0;
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
                     GL_API::s_maxTextureUnits);
    // Query shading language version support
    Console::printfn(Locale::get(_ID("GL_GLSL_SUPPORT")),
                     glGetString(GL_SHADING_LANGUAGE_VERSION));
    // GPU info, including vendor, gpu and driver
    Console::printfn(Locale::get(_ID("GL_VENDOR_STRING")),
                     gpuVendorStr, gpuRendererStr, glGetString(GL_VERSION));
    // In order: Maximum number of uniform buffer binding points,
    //           maximum size in basic machine units of a uniform block and
    //           minimum required alignment for uniform buffer sizes and offset
    GL_API::s_UBOffsetAlignment = GLUtil::getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    GL_API::s_UBMaxSize = GLUtil::getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE);
    Console::printfn(Locale::get(_ID("GL_UBO_INFO")),
                     GLUtil::getIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS),
                     GL_API::s_UBMaxSize / 1024,
                     GL_API::s_UBOffsetAlignment);

    // In order: Maximum number of shader storage buffer binding points,
    //           maximum size in basic machine units of a shader storage block,
    //           maximum total number of active shader storage blocks that may
    //           be accessed by all active shaders and
    //           minimum required alignment for shader storage buffer sizes and
    //           offset.
    GL_API::s_SSBOffsetAlignment = GLUtil::getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT);
    GL_API::s_SSBMaxSize = GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
    Console::printfn(
        Locale::get(_ID("GL_SSBO_INFO")),
        GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
        (GLUtil::getIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE) / 1024) / 1024,
        GLUtil::getIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
        GL_API::s_SSBOffsetAlignment);

    // Maximum number of subroutines and maximum number of subroutine uniform
    // locations usable in a shader
    Console::printfn(Locale::get(_ID("GL_SUBROUTINE_INFO")),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINES),
                     GLUtil::getIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS));

    // Seamless cubemaps are a nice feature to have enabled (core since 3.2)
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    //glEnable(GL_FRAMEBUFFER_SRGB);
    // Enable multisampling if we actually support and request it
    config.rendering.msaaSamples > 0 ? glEnable(GL_MULTISAMPLE)
                                     :  glDisable(GL_MULTISAMPLE);

    // Line smoothing should almost always be used
    if (Config::USE_HARDWARE_AA_LINES) {
        glEnable(GL_LINE_SMOOTH);
        glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, &s_lineWidthLimit);
    } else {
        glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, &s_lineWidthLimit);
    }

    // Culling is enabled by default, but RenderStateBlocks can toggle it on a per-draw call basis
    glEnable(GL_CULL_FACE);

    // Prepare font rendering subsystem
    if (!createFonsContext()) {
        Console::errorfn(Locale::get(_ID("ERROR_FONT_INIT")));
        return ErrorCode::FONT_INIT_ERROR;
    }

    // Prepare immediate mode emulation rendering
    NS_GLIM::glim.SetVertexAttribLocation(to_base(AttribLocation::VERTEX_POSITION));
    // Initialize our VAO pool
    GL_API::s_vaoPool.init(g_maxVAOS);
    // Initialize our query pool
    GL_API::s_hardwareQueryPool->init(g_maxQueryRings);
    // Initialize shader buffers
    glUniformBuffer::onGLInit();
    // We need a dummy VAO object for point rendering
    s_dummyVAO = GL_API::s_vaoPool.allocate();

    // In debug, we also have various performance counters to profile GPU rendering
    // operations
    if (Config::ENABLE_GPU_VALIDATION) {
        _elapsedTimeQuery->initQueries();
    }
    
    // Once OpenGL is ready for rendering, init CEGUI
    _GUIGLrenderer = &CEGUI::OpenGL3Renderer::create();
    _GUIGLrenderer->enableExtraStateSettings(config.gui.cegui.extraStates);
    CEGUI::System::create(*_GUIGLrenderer);

    if (Config::USE_ANT_TWEAK_BAR) {
        TwInit(TW_OPENGL_CORE, NULL);
    }

    static const vec4<F32> clearColour = DefaultColours::DIVIDE_BLUE();
    glClearColor(clearColour.r, clearColour.g, clearColour.b, clearColour.a);

    // Prepare shader headers and various shader related states
    if (initShaders()) {
        // That's it. Everything should be ready for draw calls
        Console::printfn(Locale::get(_ID("START_OGL_API_OK")));

        return ErrorCode::NO_ERR;
    }

    return ErrorCode::GLSL_INIT_ERROR;
}

/// Clear everything that was setup in initRenderingAPI()
void GL_API::closeRenderingAPI() {
    if (!deInitShaders()) {
        DIVIDE_UNEXPECTED_CALL("GLSL failed to shutdown!");
    }

    CEGUI::OpenGL3Renderer::destroy(*_GUIGLrenderer);
    _GUIGLrenderer = nullptr;
    // Destroy sampler objects
    {
        WriteLock w_lock(s_samplerMapLock);
        s_samplerMap.clear();
    }
    // Destroy the text rendering system
    deleteFonsContext();
    _fonts.clear();
    if (s_dummyVAO > 0) {
        glDeleteVertexArrays(1, &s_dummyVAO);
        s_dummyVAO = 0;
    }
    glVertexArray::cleanup();
    GLUtil::clearVBOs();
    GL_API::s_vaoPool.destroy();
    GL_API::s_hardwareQueryPool->destroy();
    MemoryManager::DELETE(s_hardwareQueryPool);

    destroyGLContext();
}

void GL_API::createOrValidateContextForCurrentThread(GFXDevice& context) {
    if (Runtime::isMainThread() || glbinding::getCurrentContext() != 0) {
        return;
    }

    // Double check so that we don't run into a race condition!
    UpgradableReadLock r_lock(GLUtil::_glSecondaryContextMutex);
    if (glbinding::getCurrentContext() == 0) {
        UpgradeToWriteLock w_lock(r_lock);
        // This also makes the context current
        assert(GLUtil::_glSecondaryContext == nullptr && "GL_API::syncToThread: double init context for current thread!");
        bool ctxFound = g_ContextPool.getAvailableContext(GLUtil::_glSecondaryContext);
        assert(ctxFound && "GL_API::syncToThread: context not found for current thread!");
        ACKNOWLEDGE_UNUSED(ctxFound);
        const DisplayWindow& window = context.parent().platformContext().app().windowManager().getActiveWindow();
        SDL_GL_MakeCurrent(window.getRawWindow(), GLUtil::_glSecondaryContext);
        glbinding::Binding::initialize(false);
        // Enable OpenGL debug callbacks for this context as well
        if (Config::ENABLE_GPU_VALIDATION) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            // Debug callback in a separate thread requires a flag to distinguish it
            // from the main thread's callbacks
            glDebugMessageCallback((GLDEBUGPROC)GLUtil::DebugCallback, GLUtil::_glSecondaryContext);
        }
    }
}

};
