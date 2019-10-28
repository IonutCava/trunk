#include "stdafx.h"

#include "Headers/GLWrapper.h"
#include "Headers/glIMPrimitive.h"
#include "Headers/glHardwareQuery.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/File/Headers/FileManagement.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/RenderBackend/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"

#include "GUI/Headers/GUIText.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Utility/Headers/Localization.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Lighting/Headers/LightPool.h"

#ifndef GLFONTSTASH_IMPLEMENTATION
#define GLFONTSTASH_IMPLEMENTATION
#define FONTSTASH_IMPLEMENTATION
#include "Text/Headers/fontstash.h"
#include "Text/Headers/glfontstash.h"
#endif

#include <SDL_video.h>

#include <CEGUI/CEGUI.h>
#include <GL3Renderer.h>
#include <Texture.h>

#define MAX_IMGUI_VERTS 65535 

namespace Divide {

namespace {
    bool g_frameTimeRequested = false;

    vectorFast<BufferWriteData> g_bufferLockData;
};

GLConfig GL_API::s_glConfig;
GLStateTracker* GL_API::s_activeStateTracker = nullptr;
GL_API::StateTrackerMap GL_API::s_stateTrackers;
bool GL_API::s_glFlushQueued = false;
bool GL_API::s_enabledDebugMSGGroups = false;
GLUtil::glTexturePool GL_API::s_texturePool;
glGlobalLockManager GL_API::s_globalLockManager;
GL_API::IMPrimitivePool GL_API::s_IMPrimitivePool;
moodycamel::ConcurrentQueue<BufferWriteData> GL_API::s_bufferBinds;

U8 GL_API::s_syncDeleteQueueIndexR = 1;
U8 GL_API::s_syncDeleteQueueIndexW = 0;
moodycamel::ConcurrentQueue<GLsync> GL_API::s_syncDeleteQueue[s_syncDeleteQueueSize];

GL_API::GL_API(GFXDevice& context, const bool glES)
    : RenderAPIWrapper(),
      _context(context),
      _prevSizeNode(0),
      _prevSizeString(0),
      _prevWidthNode(0),
      _prevWidthString(0),
      _commandBufferOffset(0u),
      _fonsContext(nullptr),
      _GUIGLrenderer(nullptr),
      _glswState(-1),
      _swapBufferTimer(Time::ADD_TIMER("Swap Buffer Timer"))
{
    // Only updated in Debug builds
    FRAME_DURATION_GPU = 0.f;
    _fontCache.second = -1;

    s_glConfig._glES = glES;
}

GL_API::~GL_API()
{
}

/// FontStash library initialization
bool GL_API::createFonsContext() {
    // 512x512 atlas with bottom-left origin
    _fonsContext = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);
    return _fonsContext != nullptr;
}

/// FontStash library deinitialization
void GL_API::deleteFonsContext() {
    glfonsDelete(_fonsContext);
    _fonsContext = nullptr;
}

void GL_API::idle() {
    //s_texturePool.clean();
    //s_texture2DPool.clean();
    //s_texture2DMSPool.clean();
    //s_textureCubePool.clean();
}

/// Prepare the GPU for rendering a frame
void GL_API::beginFrame(DisplayWindow& window, bool global) {
    // Start a duration query in debug builds
    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        GLuint writeQuery = _elapsedTimeQuery->writeQuery().getID();
        glBeginQuery(GL_TIME_ELAPSED, writeQuery);
    }

    GLStateTracker& stateTracker = getStateTracker();

    // Clear our buffers
    if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
        std::pair<I64, SDL_GLContext> targetContext = std::make_pair(window.getGUID(), (SDL_GLContext)window.userData());
        if (targetContext.second != nullptr && targetContext != _currentContext) {
            SDL_GL_MakeCurrent(window.getRawWindow(), targetContext.second);
            _currentContext = targetContext;
        }

        bool shouldClearColour = false, shouldClearDepth = false;
        stateTracker.setClearColour(window.clearColour(shouldClearColour, shouldClearDepth));
        ClearBufferMask mask = ClearBufferMask::GL_NONE_BIT;
        if (shouldClearColour) {
            mask |= ClearBufferMask::GL_COLOR_BUFFER_BIT;
        }
        if (shouldClearDepth) {
            mask |= ClearBufferMask::GL_DEPTH_BUFFER_BIT;
        }
        if (false) {
            mask |= ClearBufferMask::GL_STENCIL_BUFFER_BIT;
        }

        glClear(mask);
    }
    // Clears are registered as draw calls by most software, so we do the same
    // to stay in sync with third party software
    _context.registerDrawCall();

    s_stateTrackers[window.getGUID()].init(&stateTracker);

    clearStates(window, stateTracker, global);
}

/// Finish rendering the current frame
void GL_API::endFrame(DisplayWindow& window, bool global) {
    // Revert back to the default OpenGL states
    //clearStates(window, global);

    // End the timing query started in beginFrame() in debug builds
    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        glEndQuery(GL_TIME_ELAPSED);
    }
    // Swap buffers
    {
        if (global) {
            _swapBufferTimer.start();
        }

        if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
            std::pair<I64, SDL_GLContext> targetContext = std::make_pair(window.getGUID(), (SDL_GLContext)window.userData());
            if (targetContext.second != nullptr && targetContext != _currentContext) {
                SDL_GL_MakeCurrent(window.getRawWindow(), targetContext.second);
                _currentContext = targetContext;
            }

            SDL_GL_SwapWindow(window.getRawWindow());
        }

        if (global) {
            _swapBufferTimer.stop();
            s_texturePool.onFrameEnd();
            s_globalLockManager.clean(_context.getFrameCount());
            processSyncDeleteQeueue();
            s_glFlushQueued = false;

            s_syncDeleteQueueIndexR = (s_syncDeleteQueueIndexR + 1) % s_syncDeleteQueueSize;
            s_syncDeleteQueueIndexW = (s_syncDeleteQueueIndexW + 1) % s_syncDeleteQueueSize;
        }
    }

    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        // The returned results are 'g_performanceQueryRingLength - 1' frames old!
        GLuint readQuery = _elapsedTimeQuery->readQuery().getID();
        GLint available = 0;
        glGetQueryObjectiv(readQuery, GL_QUERY_RESULT_AVAILABLE, &available);

        if (available) {
            GLuint time = 0;
            glGetQueryObjectuiv(readQuery, GL_QUERY_RESULT, &time);
            FRAME_DURATION_GPU = Time::NanosecondsToMilliseconds<F32>(time);
        }

        _elapsedTimeQuery->incQueue();

        g_frameTimeRequested = false;

    }
}

F32 GL_API::getFrameDurationGPU() const {
    g_frameTimeRequested = true;
    return FRAME_DURATION_GPU;
}

void GL_API::appendToShaderHeader(ShaderType type,
                                  const stringImpl& entry,
                                  ShaderOffsetArray& inOutOffset) {
    GLuint index = to_U32(type);
    stringImpl stage;

    switch (type) {

        case ShaderType::VERTEX:
            stage = "Vertex";
            break;

        case ShaderType::FRAGMENT:
            stage = "Fragment";
            break;

        case ShaderType::GEOMETRY:
            stage = "Geometry";
            break;

        case ShaderType::TESSELLATION_CTRL:
            stage = "TessellationC";
            break;

        case ShaderType::TESSELLATION_EVAL:
            stage = "TessellationE";
            break;
        case ShaderType::COMPUTE:
            stage = "Compute";
            break;

        case ShaderType::COUNT:
            stage.clear();
            break;

        default:
            return;
    }

    glswAddDirectiveToken(stage.c_str(), entry.c_str());

    // include directives are handles differently
    if (entry.find("#include") == stringImpl::npos) {
        inOutOffset[index] += Util::LineCount(entry);
    } else {
        vector<Str64> tempAtoms;
        tempAtoms.reserve(10);
        inOutOffset[index] += Util::LineCount(glShaderProgram::preprocessIncludes("header", entry, 0, tempAtoms, true));
    }
}

/// Prepare our shader loading system
bool GL_API::initShaders() {
    glShaderProgram::initStaticData();
    return true;
}

bool GL_API::initGLSW(Configuration& config) {
    static const std::pair<stringImpl, stringImpl>  shaderVaryings[] =
    {
        { "vec4"       , "_vertexW"},
        { "vec4"       , "_vertexWV"},
        { "vec3"       , "_normalWV"},
        { "flat uvec3" , "dvd_drawParams"},
        { "vec2"       , "_texCoord"}
    };
    static const stringImpl drawParams = ""
        "#define dvd_baseInstance dvd_drawParams.x\n"
        "#define dvd_instanceID dvd_drawParams.y\n"
        "#define dvd_drawID dvd_drawParams.z\n";

    static const std::pair<stringImpl, stringImpl> shaderVaryingsBump[] =
    {
        { "mat3" , "_tbn"}
    };

    static const stringImpl crossTypeGLSLHLSL = "#define float2 vec2\n"
                                                "#define float3 vec3\n"
                                                "#define float4 vec4\n"
                                                "#define int2 ivec2\n"
                                                "#define int3 ivec3\n"
                                                "#define int4 ivec4\n"
                                                "#define float2x2 mat2\n"
                                                "#define float3x3 mat3\n"
                                                "#define float4x4 mat4\n"
                                                "#define lerp mix";

    auto getPassData = [](ShaderType type) -> stringImpl {
        stringImpl baseString = "     _out.%s = _in[index].%s;";
        if (type == ShaderType::TESSELLATION_CTRL) {
            baseString = "    _out[gl_InvocationID].%s = _in[index].%s;";
        }

        stringImpl passData("void PassData(in int index) {");
        passData.append("\n");
        for (const std::pair<stringImpl, stringImpl>& var : shaderVaryings) {
            passData.append(Util::StringFormat(baseString.c_str(), var.second.c_str(), var.second.c_str()));
            passData.append("\n");
        }
        passData.append("#if defined(COMPUTE_TBN)\n");
        for (const std::pair<stringImpl, stringImpl>& var : shaderVaryingsBump) {
            passData.append(Util::StringFormat(baseString.c_str(), var.second.c_str(), var.second.c_str()));
            passData.append("\n");
        }
        passData.append("#endif\n");

            
        passData.append("}\n");

        return passData;
    };

    // Initialize GLSW
    GLint glswState = -1;
    if (!glswGetCurrentContext()) {
        glswState = glswInit();
        DIVIDE_ASSERT(glswState == 1);
    }

    I32 numLightsPerTile = config.rendering.numLightsPerScreenTile;
    if (numLightsPerTile < 0) {
        numLightsPerTile = to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE);
    } else {
        numLightsPerTile = std::min(numLightsPerTile, to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE));
    }
    config.rendering.numLightsPerScreenTile = numLightsPerTile;

    CLAMP(config.rendering.lightThreadGroupSize, to_U8(0), to_U8(2));
    const U8 tileSize = Light::GetThreadGroupSize(config.rendering.lightThreadGroupSize);

    ShaderOffsetArray lineOffsets = { 0 };

    // Add our engine specific defines and various code pieces to every GLSL shader
    // Add version as the first shader statement, followed by copyright notice
    GLint minGLVersion = GLUtil::getGLValue(GL_MINOR_VERSION);
    GLint maxClipCull = GLUtil::getGLValue(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES);

    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#version 4%d0 core", minGLVersion), lineOffsets);

    appendToShaderHeader(ShaderType::COUNT, "/*Copyright 2009-2019 DIVIDE-Studio*/", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_shader_draw_parameters : require", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_cull_distance : require", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_gpu_shader5 : require", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_enhanced_layouts : require", lineOffsets);
    
    appendToShaderHeader(ShaderType::COUNT, crossTypeGLSLHLSL, lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define MSAA_SAMPLES %d", config.rendering.msaaSamples), lineOffsets);

    // Add current build environment information to the shaders
    if (Config::Build::IS_DEBUG_BUILD) {
        appendToShaderHeader(ShaderType::COUNT, "#define _DEBUG", lineOffsets);
    } else if (Config::Build::IS_PROFILE_BUILD) {
        appendToShaderHeader(ShaderType::COUNT, "#define _PROFILE", lineOffsets);
    } else {
        appendToShaderHeader(ShaderType::COUNT, "#define _RELEASE", lineOffsets);
    }

    // Shader stage level reflection system. A shader stage must know what stage it's used for
    appendToShaderHeader(ShaderType::VERTEX,   "#define VERT_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "#define FRAG_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "#define GEOM_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::COMPUTE,  "#define COMPUTE_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#define TESS_EVAL_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#define TESS_CTRL_SHADER", lineOffsets);

    // This line gets replaced in every shader at load with the custom list of defines specified by the material
    appendToShaderHeader(ShaderType::COUNT, "//__CUSTOM_DEFINES__", lineOffsets);

    // Add some nVidia specific pragma directives
    if (GFXDevice::getGPUVendor() == GPUVendor::NVIDIA) {
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option fastmath on", lineOffsets);
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option fastprecision on",lineOffsets);
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option inline all", lineOffsets);
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option ifcvt none", lineOffsets);
        if (Config::ENABLE_GPU_VALIDATION) {
            appendToShaderHeader(ShaderType::COUNT, "#pragma option strict on", lineOffsets);
        }
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option unroll all", lineOffsets);
    }

    if (Config::USE_COLOURED_WOIT) {
        appendToShaderHeader(ShaderType::COUNT, "#define USE_COLOURED_WOIT", lineOffsets);
    }
    appendToShaderHeader(ShaderType::COUNT,    "#define MAX_CSM_SPLITS_PER_LIGHT " + to_stringImpl(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT), lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,    "#define MAX_SHADOW_CASTING_LIGHTS " + to_stringImpl(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS), lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,    "#define MAX_LIGHTS " + to_stringImpl(Config::Lighting::MAX_POSSIBLE_LIGHTS), lineOffsets);
    
    appendToShaderHeader(ShaderType::COUNT,    "#define MAX_VISIBLE_NODES " + to_stringImpl(Config::MAX_VISIBLE_NODES), lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,    "#define Z_TEST_SIGMA 0.0001f", lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "#define DEPTH_EXP_WARP 32;", lineOffsets);
    appendToShaderHeader(ShaderType::VERTEX,   "#define MAX_BONE_COUNT_PER_NODE " + to_stringImpl(Config::MAX_BONE_COUNT_PER_NODE), lineOffsets);
    static_assert(Config::MAX_BONE_COUNT_PER_NODE <= 1024, "GLWrapper error: too many bones per vert. Can't fit inside UBO");

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_CLIP_PLANES " + 
        to_stringImpl(to_base(Frustum::FrustPlane::COUNT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_CULL_DISTANCES " + 
         to_stringImpl(maxClipCull - to_base(Frustum::FrustPlane::COUNT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_ACCUMULATION " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::ACCUMULATION)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_ALBEDO " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::ALBEDO)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_EXTRA " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::EXTRA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_NORMALS_AND_VELOCITY " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_REVEALAGE " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::REVEALAGE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TARGET_MODULATE " +
        to_stringImpl(to_base(GFXDevice::ScreenTargets::MODULATE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_GPU_BLOCK " +
        to_stringImpl(to_base(ShaderBufferLocation::GPU_BLOCK)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_ATOMIC_COUNTER " +
        to_stringImpl(to_base(ShaderBufferLocation::ATOMIC_COUNTER)),
        lineOffsets);
    
    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_GPU_COMMANDS " +
        to_stringImpl(to_base(ShaderBufferLocation::GPU_COMMANDS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_LIGHT_NORMAL " +
        to_stringImpl(to_base(ShaderBufferLocation::LIGHT_NORMAL)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_LIGHT_SHADOW " +
        to_stringImpl(to_base(ShaderBufferLocation::LIGHT_SHADOW)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_LIGHT_INDICES " +
        to_stringImpl(to_base(ShaderBufferLocation::LIGHT_INDICES)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_NODE_INFO " +
        to_stringImpl(to_base(ShaderBufferLocation::NODE_INFO)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_SCENE_DATA " +
        to_stringImpl(to_base(ShaderBufferLocation::SCENE_DATA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_COMMANDS " +
        to_stringImpl(to_base(ShaderBufferLocation::CMD_BUFFER)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_GRASS_DATA " +
        to_stringImpl(to_base(ShaderBufferLocation::GRASS_DATA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_TREE_DATA " +
        to_stringImpl(to_base(ShaderBufferLocation::TREE_DATA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define FORWARD_PLUS_TILE_RES " + 
        to_stringImpl(tileSize),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_LIGHTS_PER_TILE " + 
        to_stringImpl(numLightsPerTile),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_UNIT0 " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::UNIT0)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_HEIGHT " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::HEIGHTMAP)),
        lineOffsets);
    
    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_UNIT1 " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::UNIT1)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COMPUTE,
        "#define TEXTURE_UNIT1 " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::UNIT1)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_NORMALMAP " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::NORMALMAP)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_OPACITY " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::OPACITY)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_SPECULAR " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::SPECULAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_GLOSS " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::GLOSS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_ROUGHNESS " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::ROUGHNESS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_PROJECTION " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::PROJECTION)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_DEPTH_MAP " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::DEPTH)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_DEPTH_MAP_PREV " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::DEPTH_PREV)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFLECTION_PLANAR " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::REFLECTION_PLANAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFRACTION_PLANAR " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::REFRACTION_PLANAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFLECTION_CUBE " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::REFLECTION_CUBE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_PREPASS_SHADOWS " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::PREPASS_SHADOWS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_CUBE_MAP_ARRAY " +
        to_stringImpl(to_base(ShaderProgram::TextureUsage::SHADOW_CUBE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_LAYERED_MAP_ARRAY " +
        to_stringImpl(to_U32(ShaderProgram::TextureUsage::SHADOW_LAYERED)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_SINGLE_MAP_ARRAY " +
        to_stringImpl(to_U32(ShaderProgram::TextureUsage::SHADOW_SINGLE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_COUNT " +
        to_stringImpl(to_U32(ShaderProgram::TextureUsage::COUNT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_TERRAIN_DATA " +
        to_stringImpl(to_base(ShaderBufferLocation::TERRAIN_DATA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define BUFFER_BONE_TRANSFORMS " +
        to_stringImpl(to_base(ShaderBufferLocation::BONE_TRANSFORMS)),
        lineOffsets);

    // Vertex data has a fixed format
    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_POSITION " +
        to_stringImpl(to_base(AttribLocation::POSITION)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_TEXCOORD " +
        to_stringImpl(to_base(AttribLocation::TEXCOORD)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_NORMAL " +
        to_stringImpl(to_base(AttribLocation::NORMAL)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_TANGENT " +
        to_stringImpl(to_base(AttribLocation::TANGENT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_COLOR " +
        to_stringImpl(to_base(AttribLocation::COLOR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_BONE_WEIGHT " +
        to_stringImpl(to_base(AttribLocation::BONE_WEIGHT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_BONE_INDICE " +
        to_stringImpl(to_base(AttribLocation::BONE_INDICE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_WIDTH " +
        to_stringImpl(to_base(AttribLocation::WIDTH)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define ATTRIB_GENERIC " +
        to_stringImpl(to_base(AttribLocation::GENERIC)),
        lineOffsets);

    auto addVaryings = [&](ShaderType type, ShaderOffsetArray& lineOffsets, bool bump) {
        if (bump) {
            for (const std::pair<stringImpl, stringImpl>& entry : shaderVaryingsBump) {
                appendToShaderHeader(type, Util::StringFormat("    %s %s;", entry.first.c_str(), entry.second.c_str()), lineOffsets);
            }
        } else {
            for (const std::pair<stringImpl, stringImpl>& entry : shaderVaryings) {
                appendToShaderHeader(type, Util::StringFormat("    %s %s;", entry.first.c_str(), entry.second.c_str()), lineOffsets);
            }
        }
    };

    // Vertex shader output
    appendToShaderHeader(ShaderType::VERTEX, "out Data {", lineOffsets);
    addVaryings(ShaderType::VERTEX, lineOffsets, false);
    appendToShaderHeader(ShaderType::VERTEX, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::VERTEX, lineOffsets, true);
    appendToShaderHeader(ShaderType::VERTEX, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::VERTEX, "} _out;\n", lineOffsets);

    // Tessellation Control shader input
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "in Data {", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_CTRL, lineOffsets, false);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_CTRL, lineOffsets, true);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "} _in[];\n", lineOffsets);

    // Tessellation Control shader output
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "out Data {", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_CTRL, lineOffsets, false);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_CTRL, lineOffsets, true);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "} _out[];\n", lineOffsets);

    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, getPassData(ShaderType::TESSELLATION_CTRL), lineOffsets);

    // Tessellation Eval shader input
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "in Data {", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_EVAL, lineOffsets,false);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_EVAL, lineOffsets, true);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "} _in[];\n", lineOffsets);

    // Tessellation Eval shader output
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "out Data {", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_EVAL, lineOffsets, false);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::TESSELLATION_EVAL, lineOffsets, true);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "} _out;\n", lineOffsets);

    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, getPassData(ShaderType::TESSELLATION_EVAL), lineOffsets);

    // Geometry shader input
    appendToShaderHeader(ShaderType::GEOMETRY, "in Data {", lineOffsets);
    addVaryings(ShaderType::GEOMETRY, lineOffsets, false);
    appendToShaderHeader(ShaderType::GEOMETRY, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::GEOMETRY, lineOffsets, true);
    appendToShaderHeader(ShaderType::GEOMETRY, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "} _in[];\n", lineOffsets);

    // Geometry shader output
    appendToShaderHeader(ShaderType::GEOMETRY, "out Data {", lineOffsets);
    addVaryings(ShaderType::GEOMETRY, lineOffsets, false);
    appendToShaderHeader(ShaderType::GEOMETRY, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::GEOMETRY, lineOffsets, true);
    appendToShaderHeader(ShaderType::GEOMETRY, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "} _out;\n", lineOffsets);

    appendToShaderHeader(ShaderType::GEOMETRY, getPassData(ShaderType::GEOMETRY), lineOffsets);

    // Fragment shader input
    appendToShaderHeader(ShaderType::FRAGMENT, "in Data {", lineOffsets);
    addVaryings(ShaderType::FRAGMENT, lineOffsets, false);
    appendToShaderHeader(ShaderType::FRAGMENT, "#if defined(COMPUTE_TBN)", lineOffsets);
    addVaryings(ShaderType::FRAGMENT, lineOffsets, true);
    appendToShaderHeader(ShaderType::FRAGMENT, "#endif", lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "} _in;\n", lineOffsets);

    appendToShaderHeader(ShaderType::VERTEX, "#define VAR _out", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_CTRL, "#define VAR _in[gl_InvocationID]", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELLATION_EVAL, "#define VAR _in", lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "#define VAR _in", lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "#define VAR _in", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, drawParams, lineOffsets);

    // GPU specific data, such as GFXDevice's main uniform block and clipping
    // planes are defined in an external file included in every shader
    appendToShaderHeader(ShaderType::COUNT, "#include \"nodeDataInput.cmn\"", lineOffsets);

    Attorney::GLAPIShaderProgram::setGlobalLineOffset(lineOffsets[to_base(ShaderType::COUNT)]);

    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::VERTEX,   lineOffsets[to_base(ShaderType::VERTEX)]);
    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::TESSELLATION_CTRL, lineOffsets[to_base(ShaderType::TESSELLATION_CTRL)]);
    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::TESSELLATION_EVAL, lineOffsets[to_base(ShaderType::TESSELLATION_EVAL)]);
    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::GEOMETRY, lineOffsets[to_base(ShaderType::GEOMETRY)]);
    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::FRAGMENT, lineOffsets[to_base(ShaderType::FRAGMENT)]);
    Attorney::GLAPIShaderProgram::addLineOffset(ShaderType::COMPUTE,  lineOffsets[to_base(ShaderType::COMPUTE)]);

    // Check initialization status for GLSL and glsl-optimizer
    return glswState == 1;
}

/// Revert everything that was set up in "initShaders()"
bool GL_API::deInitShaders() {
    glShaderProgram::destroyStaticData();
    return true;
}

bool GL_API::deInitGLSW() {
    // Shutdown GLSW
    return glswShutdown() == 1;
}
/// Try to find the requested font in the font cache. Load on cache miss.
I32 GL_API::getFont(const Str64& fontName) {
    if (_fontCache.first.compare(fontName) != 0) {
        _fontCache.first = fontName;
        U64 fontNameHash = _ID(fontName.c_str());
        // Search for the requested font by name
        FontCache::const_iterator it = _fonts.find(fontNameHash);
        // If we failed to find it, it wasn't loaded yet
        if (it == std::cend(_fonts)) {
            // Fonts are stored in the general asset directory -> in the GUI
            // subfolder -> in the fonts subfolder
            Str256 fontPath(Paths::g_assetsLocation + Paths::g_GUILocation + Paths::g_fontsPath);
            fontPath += fontName.c_str();
            // We use FontStash to load the font file
            _fontCache.second = fonsAddFont(_fonsContext, fontName.c_str(), fontPath.c_str());
            // If the font is invalid, inform the user, but map it anyway, to avoid
            // loading an invalid font file on every request
            if (_fontCache.second == FONS_INVALID) {
                Console::errorfn(Locale::get(_ID("ERROR_FONT_FILE")), fontName.c_str());
            }
            // Save the font in the font cache
            hashAlg::insert(_fonts, fontNameHash, _fontCache.second);
            
        } else {
            _fontCache.second = it->second;
        }

    }

    // Return the font
    return _fontCache.second;
}

/// Text rendering is handled exclusively by Mikko Mononen's FontStash library (https://github.com/memononen/fontstash)
/// with his OpenGL frontend adapted for core context profiles
void GL_API::drawText(const TextElementBatch& batch) {

    pushDebugMessage("OpenGL render text start!", 2);

    BlendingProperties blend = {
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA,
        BlendOperation::ADD
    };
    blend._enabled = true;

    getStateTracker().setBlending(0, blend);

    getStateTracker().setBlendColour(DefaultColours::DIVIDE_BLUE_U8);
    I32 width = _context.renderingResolution().w;
    I32 height = _context.renderingResolution().h;
        
    vec_size drawCount = 0;

    fonsClearState(_fonsContext);
    size_t previousStyle = 0;
    for (const TextElement& entry : batch())
    {
        if (previousStyle != entry._textLabelStyleHash) {
            const TextLabelStyle& textLabelStyle = TextLabelStyle::get(entry._textLabelStyleHash);
            // Retrieve the font from the font cache
            I32 font = getFont(TextLabelStyle::fontName(textLabelStyle.font()));
            // The font may be invalid, so skip this text label
            if (font != FONS_INVALID) {
                fonsSetFont(_fonsContext, font);
            }
            fonsSetBlur(_fonsContext, textLabelStyle.blurAmount());
            fonsSetBlur(_fonsContext, textLabelStyle.spacing());
            fonsSetAlign(_fonsContext, textLabelStyle.alignFlag());
            fonsSetSize(_fonsContext, to_F32(textLabelStyle.fontSize()));
            fonsSetColour(_fonsContext,
                          textLabelStyle.colour().r,
                          textLabelStyle.colour().g,
                          textLabelStyle.colour().b,
                          textLabelStyle.colour().a);
            previousStyle = entry._textLabelStyleHash;
        }

        F32 textX = entry._position.d_x.d_scale * width + entry._position.d_x.d_offset;
        F32 textY = entry._position.d_y.d_scale * height + entry._position.d_y.d_offset;

        textY = height - textY;
        F32 lh = 0;
        fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);
        
        const TextElement::TextType& text = entry.text();
        vec_size_eastl lineCount = text.size();
        for (vec_size_eastl i = 0; i < lineCount; ++i) {
            fonsDrawText(_fonsContext,
                         textX,
                         textY - (lh * i),
                         text[i].c_str(),
                         nullptr);
        }
        drawCount += lineCount;
        

        // Register each label rendered as a draw call
        _context.registerDrawCalls(to_U32(drawCount));
    }

    popDebugMessage();
}

void GL_API::drawIMGUI(ImDrawData* data, I64 windowGUID) {
    if (data != nullptr && data->Valid) {

        GenericVertexData::IndexBuffer idxBuffer;
        GenericDrawCommand cmd = {};
        cmd._primitiveType = PrimitiveType::TRIANGLES;

        ImVec2 pos = data->DisplayPos;
        for (int n = 0; n < data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = data->CmdLists[n];
            U32 vertCount = to_U32(cmd_list->VtxBuffer.size());
            assert(vertCount < MAX_IMGUI_VERTS);

            cmd._cmd.firstIndex = 0;

            idxBuffer.smallIndices = sizeof(ImDrawIdx) == 2;
            idxBuffer.count = to_U32(cmd_list->IdxBuffer.Size);
            idxBuffer.data = (bufferPtr)cmd_list->IdxBuffer.Data;

            GenericVertexData* buffer = getOrCreateIMGUIBuffer(windowGUID);
            assert(buffer != nullptr);

            buffer->updateBuffer(0u, vertCount, 0u, cmd_list->VtxBuffer.Data);
            buffer->updateIndexBuffer(idxBuffer);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

                if (pcmd->UserCallback) {
                    // User callback (registered via ImDrawList::AddCallback)
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    const Rect<I32>& viewport = s_activeStateTracker->_activeViewport;
                    Rect<I32> clip_rect = {
                        pcmd->ClipRect.x - pos.x,
                        pcmd->ClipRect.y - pos.y,
                        pcmd->ClipRect.z - pos.x,
                        pcmd->ClipRect.w - pos.y
                    };

                    if (clip_rect.x < viewport.z &&
                        clip_rect.y < viewport.w &&
                        clip_rect.z >= 0 &&
                        clip_rect.w >= 0)
                    {
                        const I32 tempW = clip_rect.w;
                        clip_rect.z -= clip_rect.x;
                        clip_rect.w -= clip_rect.y;
                        clip_rect.y  = viewport.w - tempW;
                        getStateTracker().setScissor(clip_rect);
                        cmd._cmd.indexCount = to_U32(pcmd->ElemCount);

                        s_activeStateTracker->bindTexture(0, TextureType::TEXTURE_2D, (GLuint)((intptr_t)pcmd->TextureId));
                        buffer->draw(cmd, 0);
                    }
                }
                cmd._cmd.firstIndex += pcmd->ElemCount;
            }
        }
    }
}

bool GL_API::bindPipeline(const Pipeline& pipeline) {
    if (s_activeStateTracker->_activePipeline && *s_activeStateTracker->_activePipeline == pipeline) {
        return true;
    }

    s_activeStateTracker->_activePipeline = &pipeline;

    // Set the proper render states
    setStateBlock(pipeline.stateHash());

    ShaderProgram* program = ShaderProgram::findShaderProgram(pipeline.shaderProgramHandle());
    if (program == nullptr) {
        // Should we return false?
        program = ShaderProgram::defaultShader().get();
    }

    glShaderProgram& glProgram = static_cast<glShaderProgram&>(*program);
    // We need a valid shader as no fixed function pipeline is available
    
    // Try to bind the shader program. If it failed to load, or isn't loaded yet, cancel the draw request for this frame
    bool wasBound = false;
    if (Attorney::GLAPIShaderProgram::bind(glProgram, wasBound)) {
        const ShaderFunctions& functions = pipeline.shaderFunctions();
        for (U8 type = 0; type < to_U8(ShaderType::COUNT); ++type) {
            Attorney::GLAPIShaderProgram::SetSubroutines(glProgram, static_cast<ShaderType>(type), functions[type]);
        }
        if (!wasBound) {
            Attorney::GLAPIShaderProgram::queueValidation(glProgram);
        }
        return true;
    }

    return false;
}

void GL_API::sendPushConstants(const PushConstants& pushConstants) {
    assert(s_activeStateTracker->_activePipeline != nullptr);

    ShaderProgram* program = ShaderProgram::findShaderProgram(s_activeStateTracker->_activePipeline->shaderProgramHandle());
    if (program == nullptr) {
        // Should we skip the upload?
        program = ShaderProgram::defaultShader().get();
    }

    static_cast<glShaderProgram*>(program)->UploadPushConstants(pushConstants);
}

bool GL_API::draw(const GenericDrawCommand& cmd, U32 cmdBufferOffset) {
    if (cmd._sourceBuffer._id == 0) {
        getStateTracker().setActiveVAO(s_dummyVAO);

        U32 indexCount = 0;
        switch (cmd._primitiveType) {
            case PrimitiveType::TRIANGLES: indexCount = cmd._drawCount * 3; break;
            case PrimitiveType::API_POINTS: indexCount = cmd._drawCount; break;
            default: indexCount = cmd._cmd.indexCount; break;
        }

        glDrawArrays(GLUtil::glPrimitiveTypeTable[to_U32(cmd._primitiveType)], cmd._cmd.firstIndex, indexCount);
    } else {
        VertexDataInterface* buffer = VertexDataInterface::s_VDIPool.find(cmd._sourceBuffer);
        assert(buffer != nullptr);
        buffer->draw(cmd, cmdBufferOffset);
    }

    return true;
}

void GL_API::pushDebugMessage(const char* message, I32 id) {
    if (Config::ENABLE_GPU_VALIDATION && s_enabledDebugMSGGroups) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, message);
    }
}

void GL_API::popDebugMessage() {
    if (Config::ENABLE_GPU_VALIDATION && s_enabledDebugMSGGroups) {
        glPopDebugGroup();
    }
}

namespace {
    static bool s_firstCommandInBuffer = true;
};

void GL_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {
    switch (static_cast<GFX::CommandType>(entry._typeIndex)) {
        case GFX::CommandType::BEGIN_RENDER_PASS: {
            const GFX::BeginRenderPassCommand& crtCmd = commandBuffer.get<GFX::BeginRenderPassCommand>(entry);
            glFramebuffer& rt = static_cast<glFramebuffer&>(_context.renderTargetPool().renderTarget(crtCmd._target));
            Attorney::GLAPIRenderTarget::begin(rt, crtCmd._descriptor);
            s_activeStateTracker->_activeRenderTarget = &rt;
            GL_API::pushDebugMessage(crtCmd._name.c_str(), std::numeric_limits<I32>::max());
        }break;
        case GFX::CommandType::END_RENDER_PASS: {
            const GFX::EndRenderPassCommand& crtCmd = commandBuffer.get<GFX::EndRenderPassCommand>(entry);
            assert(s_activeStateTracker->_activeRenderTarget != nullptr);
            GL_API::popDebugMessage();
            glFramebuffer& fb = *s_activeStateTracker->_activeRenderTarget;
            Attorney::GLAPIRenderTarget::end(fb, crtCmd._autoResolveMSAAColour, crtCmd._autoResolveMSAAExternalColour, crtCmd._autoResolveMSAADepth);
        }break;
        case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
            const GFX::BeginPixelBufferCommand& crtCmd = commandBuffer.get<GFX::BeginPixelBufferCommand>(entry);
            assert(crtCmd._buffer != nullptr);
            glPixelBuffer* buffer = static_cast<glPixelBuffer*>(crtCmd._buffer);
            bufferPtr data = Attorney::GLAPIPixelBuffer::begin(*buffer);
            if (crtCmd._command) {
                crtCmd._command(data);
            }
            s_activeStateTracker->_activePixelBuffer = buffer;
        }break;
        case GFX::CommandType::END_PIXEL_BUFFER: {
            assert(s_activeStateTracker->_activePixelBuffer != nullptr);
            Attorney::GLAPIPixelBuffer::end(*s_activeStateTracker->_activePixelBuffer);
        }break;
        case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
            assert(s_activeStateTracker->_activeRenderTarget != nullptr);
            const GFX::BeginRenderSubPassCommand& crtCmd = commandBuffer.get<GFX::BeginRenderSubPassCommand>(entry);
            for (const RenderTarget::DrawLayerParams& params : crtCmd._writeLayers) {
                s_activeStateTracker->_activeRenderTarget->drawToLayer(params);
            }

            s_activeStateTracker->_activeRenderTarget->setMipLevel(crtCmd._mipWriteLevel, crtCmd._validateWriteLevel);
        }break;
        case GFX::CommandType::END_RENDER_SUB_PASS: {
        }break;
        case GFX::CommandType::RESOLVE_RT: {
            const GFX::ResolveRenderTargetCommand& crtCmd = commandBuffer.get<GFX::ResolveRenderTargetCommand>(entry);
            glFramebuffer& rt = static_cast<glFramebuffer&>(_context.renderTargetPool().renderTarget(crtCmd._source));
            Attorney::GLAPIRenderTarget::resolve(rt, crtCmd._resolveColour, crtCmd._resolveColours, crtCmd._resolveDepth, crtCmd._resolveExternalColours);
        }break;
        case GFX::CommandType::COPY_TEXTURE: {
            const GFX::CopyTextureCommand& crtCmd = commandBuffer.get<GFX::CopyTextureCommand>(entry);
            glTexture::copy(crtCmd._source, crtCmd._destination, crtCmd._params);
        }break;
        case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
            const GFX::BindDescriptorSetsCommand& crtCmd = commandBuffer.get<GFX::BindDescriptorSetsCommand>(entry);
            const DescriptorSet& set = crtCmd._set;

            if (makeTexturesResident(set._textureData, set._textureViews)) {
                
            }
            if (makeImagesResident(set._images)) {

            }
            for (const ShaderBufferBinding& shaderBufCmd : set._shaderBuffers) {
                glUniformBuffer* buffer = static_cast<glUniformBuffer*>(shaderBufCmd._buffer);

                if (shaderBufCmd._binding == ShaderBufferLocation::CMD_BUFFER) {
                    getStateTracker().setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, buffer->bufferID());
                    _commandBufferOffset = shaderBufCmd._elementRange.x;
                } else {
                    buffer->bindRange(to_U8(shaderBufCmd._binding), shaderBufCmd._elementRange.x, shaderBufCmd._elementRange.y);
                }
            }
        }break;
        case GFX::CommandType::BIND_PIPELINE: {
            const Pipeline* pipeline = commandBuffer.get<GFX::BindPipelineCommand>(entry)._pipeline;
            assert(pipeline != nullptr);
            bindPipeline(*pipeline);
        } break;
        case GFX::CommandType::SEND_PUSH_CONSTANTS: {
            sendPushConstants(commandBuffer.get<GFX::SendPushConstantsCommand>(entry)._constants);
        } break;
        case GFX::CommandType::SET_SCISSOR: {
            getStateTracker().setScissor(commandBuffer.get<GFX::SetScissorCommand>(entry)._rect);
        }break;
        case GFX::CommandType::SET_BLEND: {
            const GFX::SetBlendCommand& blendCmd = commandBuffer.get<GFX::SetBlendCommand>(entry);
            getStateTracker().setBlending(blendCmd._blendProperties);
        }break;
        case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
             const GFX::BeginDebugScopeCommand& crtCmd = commandBuffer.get<GFX::BeginDebugScopeCommand>(entry);
             pushDebugMessage(crtCmd._scopeName.c_str(), crtCmd._scopeID);
        } break;
        case GFX::CommandType::END_DEBUG_SCOPE: {
            popDebugMessage();
        } break;
        case GFX::CommandType::COMPUTE_MIPMAPS: {
            const GFX::ComputeMipMapsCommand& crtCmd = commandBuffer.get<GFX::ComputeMipMapsCommand>(entry);
            if (crtCmd._layerRange.x == 0 && crtCmd._layerRange.y <= 1) {
                glGenerateTextureMipmap(crtCmd._texture->data().textureHandle());
            } else {

                Texture* tex = crtCmd._texture;
                TextureData data = tex->data();
                const TextureDescriptor& descriptor = tex->descriptor();
                GLenum glInternalFormat = GLUtil::internalFormat(descriptor.baseFormat(), descriptor.dataType(), descriptor.srgb());

                GLenum type = GLUtil::glTextureTypeTable[to_base(data.type())];
                GLuint handle = s_texturePool.allocate(GL_NONE);
                glTextureView(handle,
                    type,
                    data.textureHandle(),
                    glInternalFormat,
                    (GLuint)descriptor.mipLevels().x,
                    (GLuint)descriptor.mipLevels().y,
                    (GLuint)crtCmd._layerRange.x,
                    (GLuint)crtCmd._layerRange.y);
                glGenerateTextureMipmap(handle);
                s_texturePool.deallocate(handle, GL_NONE, 4);
            }
        }break;
        case GFX::CommandType::DRAW_TEXT: {
            const GFX::DrawTextCommand& crtCmd = commandBuffer.get<GFX::DrawTextCommand>(entry);
            drawText(crtCmd._batch);
            lockBuffers(false, _context.getFrameCount());
        }break;
        case GFX::CommandType::DRAW_IMGUI: {
            const GFX::DrawIMGUICommand& crtCmd = commandBuffer.get<GFX::DrawIMGUICommand>(entry);
            drawIMGUI(crtCmd._data, crtCmd._windowGUID);
            lockBuffers(false, _context.getFrameCount());
        }break;
        case GFX::CommandType::DRAW_COMMANDS : {
            const GFX::DrawCommand& crtCmd = commandBuffer.get<GFX::DrawCommand>(entry);
            const vectorEASTLFast<GenericDrawCommand>& drawCommands = crtCmd._drawCommands;
            for (const GenericDrawCommand& currentDrawCommand : drawCommands) {
                if (draw(currentDrawCommand, _commandBufferOffset)) {
                    // Lock all buffers as soon as we issue a draw command since we should've flushed the command queue by now
                    lockBuffers(s_firstCommandInBuffer, _context.getFrameCount());
                    s_firstCommandInBuffer = false;

                    if (isEnabledOption(currentDrawCommand, CmdRenderOptions::RENDER_GEOMETRY)) {
                        if (isEnabledOption(currentDrawCommand, CmdRenderOptions::RENDER_WIREFRAME)) {
                            _context.registerDrawCalls(2);
                        } else {
                            _context.registerDrawCall();
                        }
                    }
                }
            }
        }break;
        case GFX::CommandType::DISPATCH_COMPUTE: {
            const GFX::DispatchComputeCommand& crtCmd = commandBuffer.get<GFX::DispatchComputeCommand>(entry);
            assert(s_activeStateTracker->_activePipeline != nullptr);
            glDispatchCompute(crtCmd._computeGroupSize.x, crtCmd._computeGroupSize.y, crtCmd._computeGroupSize.z);
            lockBuffers(false, _context.getFrameCount());
        }break;
        case GFX::CommandType::MEMORY_BARRIER: {
            const GFX::MemoryBarrierCommand& crtCmd = commandBuffer.get<GFX::MemoryBarrierCommand>(entry);
            MemoryBarrierMask glMask = MemoryBarrierMask::GL_NONE_BIT;
            U8 barrierMask = crtCmd._barrierMask;
            if (barrierMask != 0) {
                if (barrierMask == to_base(MemoryBarrierType::ALL)) {
                    glMemoryBarrier(MemoryBarrierMask::GL_ALL_BARRIER_BITS);
                } else {
                    for (U8 i = 0; i < to_U8(MemoryBarrierType::COUNT); ++i) {
                        if (BitCompare(barrierMask, to_U8(1 << i))) {
                            switch (static_cast<MemoryBarrierType>(1 << i)) {
                                case MemoryBarrierType::BUFFER:
                                    glMask |= MemoryBarrierMask::GL_BUFFER_UPDATE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::SHADER_BUFFER:
                                    glMask |= MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::COUNTER:
                                    glMask |= MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::QUERY:
                                    glMask |= MemoryBarrierMask::GL_QUERY_BUFFER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::RENDER_TARGET:
                                    glMask |= MemoryBarrierMask::GL_FRAMEBUFFER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::TEXTURE:
                                    glMask |= MemoryBarrierMask::GL_TEXTURE_UPDATE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::TRANSFORM_FEEDBACK:
                                    glMask |= MemoryBarrierMask::GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
                                    break;
                            }
                        }
                    }
                   glMemoryBarrier(glMask);
               }
            }
        } break;
    };
}

void GL_API::lockBuffers(bool flush, U32 frameID) {
    BufferWriteData data = {};
    bool shouldFlush = false;

    g_bufferLockData.resize(0);

    while (s_bufferBinds.try_dequeue(data)) {

        bool updatedExisting = false;
        // Buffer locking only happens on the main rendering thread (OpenGL forces us to) so we merge as many locks as possible here
        // const BufferRange testRange{ data._offset, data._range };
        for (BufferWriteData& existingData : g_bufferLockData) {
            if (existingData._handle == data._handle) {
                //const BufferRange existingRange{ existingData._offset, existingData._range };
                //if (testRange.Overlaps(existingRange)) 
                {
                    existingData._offset = std::min(existingData._offset, data._offset);
                    existingData._range = std::max(existingData._range, data._range);
                    updatedExisting = true;
                    shouldFlush = data._flush || shouldFlush;
                }
                break;
            }
        }

        if (!updatedExisting) {
            g_bufferLockData.emplace_back(data._handle, data._offset, data._range);
            shouldFlush = data._flush || shouldFlush;
        }
    }

    if (!g_bufferLockData.empty()) {
        BufferLockEntries entries;
        for (const BufferWriteData& entry : g_bufferLockData) {
            entries[entry._handle].emplace_back(BufferRange{ entry._offset, entry._range });
        }

        s_globalLockManager.LockBuffers(std::move(entries), flush && shouldFlush, frameID);
        if (!flush) {
            s_glFlushQueued = shouldFlush;
        }
    }
}

void GL_API::preFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    s_firstCommandInBuffer = true;
}

void GL_API::postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer, bool submitToGPU) {
    if (s_glFlushQueued && submitToGPU) {
        glFlush();
        s_glFlushQueued = false;
    }
}

void GL_API::registerBufferBind(BufferWriteData&& data) {
    assert(Runtime::isMainThread());

    if (data._handle == -1 || data._range == 0) {
        return;
    }

    if (!s_bufferBinds.enqueue(std::move(data))) {
        assert(false && "GL_API::registerBufferBind failure!");
    }
}

void GL_API::registerSyncDelete(GLsync syncObject) {
#if 1
    glDeleteSync(syncObject);
#else
    if (!s_syncDeleteQueue[s_syncDeleteQueueIndexW].enqueue(syncObject)) {
        assert(false && "GL_API::registerSyncDelete failure!");
    }
#endif
}

void GL_API::processSyncDeleteQeueue() {
    GLsync sync;
    while (s_syncDeleteQueue[s_syncDeleteQueueIndexR].try_dequeue(sync)) {
        glDeleteSync(sync);
    }
}

GenericVertexData* GL_API::getOrCreateIMGUIBuffer(I64 windowGUID) {
    GenericVertexData* ret = nullptr;

    auto it = _IMGUIBuffers.find(windowGUID);
    if (it == eastl::cend(_IMGUIBuffers)) {
        // Ring buffer wouldn't work properly with an IMMEDIATE MODE gui
        // We update and draw multiple times in a loop
        ret = _context.newGVD(1);

        GenericVertexData::IndexBuffer idxBuff;
        idxBuff.smallIndices = sizeof(ImDrawIdx) == 2;
        idxBuff.count = MAX_IMGUI_VERTS * 3;

        ret->create(1);

        GenericVertexData::SetBufferParams params = {};
        params._buffer = 0;
        params._elementCount = MAX_IMGUI_VERTS;
        params._elementSize = sizeof(ImDrawVert);
        params._useRingBuffer = true;
        params._updateFrequency = BufferUpdateFrequency::OFTEN;
        params._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        params._storageType = BufferStorageType::NORMAL;
        params._sync = true;
        params._data = NULL;

        ret->setBuffer(params); //Pos, UV and Colour
        ret->setIndexBuffer(idxBuff, BufferUpdateFrequency::OFTEN);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        AttributeDescriptor& descPos = ret->attribDescriptor(to_base(AttribLocation::GENERIC));
        AttributeDescriptor& descUV = ret->attribDescriptor(to_base(AttribLocation::TEXCOORD));
        AttributeDescriptor& descColour = ret->attribDescriptor(to_base(AttribLocation::COLOR));

        descPos.set(0,
                    2,
                    GFXDataFormat::FLOAT_32,
                    false,
                    to_U32(OFFSETOF(ImDrawVert, pos)));

        descUV.set(0,
                   2,
                   GFXDataFormat::FLOAT_32,
                   false,
                   to_U32(OFFSETOF(ImDrawVert, uv)));

        descColour.set(0,
                       4,
                       GFXDataFormat::UNSIGNED_BYTE,
                       true,
                       to_U32(OFFSETOF(ImDrawVert, col)));

#undef OFFSETOF
        _IMGUIBuffers[windowGUID] = ret;
    } else {
        ret = it->second;
    }

    return ret;
}

/// Activate the render state block described by the specified hash value (0 == default state block)
size_t GL_API::setStateBlock(size_t stateBlockHash) {
    // Passing 0 is a perfectly acceptable way of enabling the default render state block
    if (stateBlockHash == 0) {
        stateBlockHash = _context.getDefaultStateBlock(false);
    }

    return getStateTracker().setStateBlock(stateBlockHash);
}

bool GL_API::makeImagesResident(const vectorEASTLFast<Image>& images) {
    for (const Image& image : images) {
        if (image._texture != nullptr) {
            image._texture->bindLayer(image._binding, image._level, image._layer, false, image._flag != Image::Flag::WRITE ? true : false, image._flag != Image::Flag::READ ? true : false);
        }
    }

    return true;
}

bool GL_API::makeTexturesResident(const TextureDataContainer& textureData, const vectorEASTLFast<TextureViewEntry>& textureViews) {
    bool bound = false;

    STUBBED("ToDo: Optimise this: If over n textures, get max binding slot, create [0...maxSlot] bindings, fill unused with 0 and send as one command with glBindTextures -Ionut")
    constexpr vec_size k_textureThreshold = 3;
    size_t texCount = textureData.textures().size();
    if (texCount > k_textureThreshold && false) {
        GLushort offset = 0;
        vector<TextureType> types;
        vector<GLuint> handles;
        vector<GLuint> samplers;
        
        types.reserve(texCount);
        handles.reserve(texCount);
        samplers.reserve(texCount);

        for (const auto& data : textureData.textures()) {
            types.push_back(data.second.type());
            handles.push_back(data.second.textureHandle());
            samplers.push_back(data.second.samplerHandle());
        }

        bound = getStateTracker().bindTextures(offset, (GLuint)texCount, types.data(), handles.data(), samplers.data());
    } else {
        for (const auto& data : textureData.textures()) {
            bound = getStateTracker().bindTexture(static_cast<GLushort>(data.first),
                                                  data.second.type(),
                                                  data.second.textureHandle(),
                                                  data.second.samplerHandle()) || bound;
        }
    }

    for (auto it : textureViews) {
        Texture* tex = it._view._texture;
        const TextureData& data = tex->data();
        const TextureDescriptor& descriptor = tex->descriptor();
        GLenum glInternalFormat = GLUtil::internalFormat(descriptor.baseFormat(), descriptor.dataType(), descriptor.srgb());

        GLenum type = GLUtil::glTextureTypeTable[to_base(data.type())];
        GLuint handle = s_texturePool.allocate(GL_NONE);
        glTextureView(handle,
            type,
            data.textureHandle(),
            glInternalFormat,
            (GLuint)it._view._mipLevels.x,
            (GLuint)it._view._mipLevels.y,
            (GLuint)it._view._layerRange.x,
            (GLuint)it._view._layerRange.y);

        getStateTracker().bindTexture(static_cast<GLushort>(it._binding), data.type(), handle, data.samplerHandle());
        s_texturePool.deallocate(handle, GL_NONE, 3);
    }

    return bound;
}

bool GL_API::setViewport(const Rect<I32>& viewport) {
    return getStateTracker().setViewport(viewport);
}

/// Verify if we have a sampler object created and available for the given
/// descriptor
U32 GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor) {
    // Get the descriptor's hash value
    size_t hashValue = descriptor.getHash();
    // Try to find the hash value in the sampler object map
    GLuint sampler = getSamplerHandle(hashValue);
    if (sampler == 0) {
        UniqueLock w_lock(s_samplerMapLock);
        // Create and store the newly created sample object. GL_API is responsible for deleting these!
        sampler = glSamplerObject::construct(descriptor);
        hashAlg::emplace(s_samplerMap, hashValue, sampler);
    }

    // Return the sampler object's handle
    return to_U32(sampler);
}

/// Return the OpenGL sampler object's handle for the given hash value
GLuint GL_API::getSamplerHandle(size_t samplerHash) {
    // If the hash value is 0, we assume the code is trying to unbind a sampler object
    if (samplerHash > 0) {
        // If we fail to find the sampler object for the given hash, we print an
        // error and return the default OpenGL handle
        UniqueLock r_lock(s_samplerMapLock);
        SamplerObjectMap::const_iterator it = s_samplerMap.find(samplerHash);
        if (it != std::cend(s_samplerMap)) {
            // Return the OpenGL handle for the sampler object matching the specified hash value
            return it->second;
        }
    }

    return 0u;
}

U32 GL_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const {
    return to_U32(static_cast<const CEGUI::OpenGLTexture&>(textureIn).getOpenGLTexture());
}

IMPrimitive* GL_API::newIMP(std::mutex& lock, GFXDevice& parent) {
    return s_IMPrimitivePool.newElement(lock, parent);
}

bool GL_API::destroyIMP(std::mutex& lock, IMPrimitive*& primitive) {
    if (primitive != nullptr) {
        s_IMPrimitivePool.deleteElement(lock, static_cast<glIMPrimitive*>(primitive));
        primitive = nullptr;
        return true;
    }

    return false;
}

};
