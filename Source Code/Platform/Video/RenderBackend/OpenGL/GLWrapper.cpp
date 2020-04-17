#include "stdafx.h"

#include "Headers/GLWrapper.h"
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
};

GLConfig GL_API::s_glConfig = {};
GLStateTracker GL_API::s_stateTracker = {};
bool GL_API::s_enabledDebugMSGGroups = false;
bool GL_API::s_glFlushQueued = false;
GLUtil::glTexturePool GL_API::s_texturePool = {};
GL_API::IMPrimitivePool GL_API::s_IMPrimitivePool = {};
eastl::fixed_vector<BufferLockEntry, 64, true> GL_API::s_bufferLockQueue;

GL_API::GL_API(GFXDevice& context, const bool glES)
    : RenderAPIWrapper(),
      _context(context),
      _prevSizeNode(0),
      _prevSizeString(0),
      _prevWidthNode(0),
      _prevWidthString(0),
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
    OPTICK_EVENT();

    // Start a duration query in debug builds
    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        _elapsedTimeQuery->begin();
    }

    GLStateTracker& stateTracker = getStateTracker();

    // Clear our buffers
    if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
        SDL_GLContext glContext = (SDL_GLContext)window.userData();
        const I64 windowGUID = window.getGUID();

        if (glContext != nullptr && (_currentContext.first != windowGUID || _currentContext.second != glContext)) {
            SDL_GL_MakeCurrent(window.getRawWindow(), glContext);
            _currentContext = std::make_pair(windowGUID, glContext);
        }

        bool shouldClearColour = false, shouldClearDepth = false, shouldClearStencil = false;
        stateTracker.setClearColour(window.clearColour(shouldClearColour, shouldClearDepth));
        ClearBufferMask mask = ClearBufferMask::GL_NONE_BIT;
        if (shouldClearColour) {
            mask |= ClearBufferMask::GL_COLOR_BUFFER_BIT;
        }
        if (shouldClearDepth) {
            mask |= ClearBufferMask::GL_DEPTH_BUFFER_BIT;
        }
        if (shouldClearStencil) {
            mask |= ClearBufferMask::GL_STENCIL_BUFFER_BIT;
        }
        if (mask != ClearBufferMask::GL_NONE_BIT) {
            glClear(mask);
        }
    }
    // Clears are registered as draw calls by most software, so we do the same
    // to stay in sync with third party software
    _context.registerDrawCall();

    clearStates(window, stateTracker, global);
}

/// Finish rendering the current frame
void GL_API::endFrame(DisplayWindow& window, bool global) {
    OPTICK_EVENT();

    // Revert back to the default OpenGL states
    //clearStates(window, global);

    // End the timing query started in beginFrame() in debug builds
    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        _elapsedTimeQuery->end();
    }
    // Swap buffers
    {
        if (global) {
            _swapBufferTimer.start();
        }

        if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
            SDL_GLContext glContext = (SDL_GLContext)window.userData();
            const I64 windowGUID = window.getGUID();
            
            if (glContext != nullptr && (_currentContext.first != windowGUID || _currentContext.second != glContext)) {
                SDL_GL_MakeCurrent(window.getRawWindow(), glContext);
                _currentContext = std::make_pair(windowGUID, glContext);
            }
            {
                OPTICK_EVENT("Swap Buffers");
                SDL_GL_SwapWindow(window.getRawWindow());
            }
        }
        if (global) {
            _swapBufferTimer.stop();
            s_texturePool.onFrameEnd();
            s_glFlushQueued = false;
        }
    }

    if (global && Config::ENABLE_GPU_VALIDATION && g_frameTimeRequested) {
        // The returned results are 'g_performanceQueryRingLength - 1' frames old!
        const I64 time = _elapsedTimeQuery->getResultNoWait();
        FRAME_DURATION_GPU = Time::NanosecondsToMilliseconds<F32>(time);
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
    const GLuint index = to_U32(type);
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
        vectorSTD<Str64> tempAtoms;
        tempAtoms.reserve(10);
        inOutOffset[index] += Util::LineCount(glShaderProgram::preprocessIncludes("header", entry, 0, tempAtoms, true));
    }
}

bool GL_API::initGLSW(Configuration& config) {
    constexpr std::pair<const char*, const char*> shaderVaryings[] =
    {
        { "vec4"       , "_vertexW"},
        { "vec4"       , "_vertexWV"},
        { "vec3"       , "_normalWV"},
        { "vec3"       , "_viewDirectionWV"},
        { "flat uvec3" , "_drawParams"},
        { "vec2"       , "_texCoord"}
    };

    constexpr const char* drawParams = ""
        "#define dvd_baseInstance _drawParams.x\n"
        "#define dvd_instanceID _drawParams.y\n"
        "#define dvd_drawID _drawParams.z\n";

    constexpr std::pair<const char*, const char*> shaderVaryingsBump[] =
    {
        { "mat3" , "_tbn"}
    };

    constexpr const char* crossTypeGLSLHLSL = "#define float2 vec2\n"
                                              "#define float3 vec3\n"
                                              "#define float4 vec4\n"
                                              "#define int2 ivec2\n"
                                              "#define int3 ivec3\n"
                                              "#define int4 ivec4\n"
                                              "#define float2x2 mat2\n"
                                              "#define float3x3 mat3\n"
                                              "#define float4x4 mat4\n"
                                              "#define lerp mix";

    const auto getPassData = [&](ShaderType type) -> stringImpl {
        stringImpl baseString = "     _out.%s = _in[index].%s;";
        if (type == ShaderType::TESSELLATION_CTRL) {
            baseString = "    _out[gl_InvocationID].%s = _in[index].%s;";
        }

        stringImpl passData("void PassData(in int index) {");
        passData.append("\n");
        for (U8 i = 0; i < (sizeof(shaderVaryings) / sizeof(shaderVaryings[0])); ++i) {
            passData.append(Util::StringFormat(baseString.c_str(), shaderVaryings[i].second, shaderVaryings[i].second));
            passData.append("\n");
        }
        passData.append("#if defined(COMPUTE_TBN)\n");
        for (U8 i = 0; i < (sizeof(shaderVaryingsBump) / sizeof(shaderVaryingsBump[0])); ++i) {
            passData.append(Util::StringFormat(baseString.c_str(), shaderVaryingsBump[i].second, shaderVaryingsBump[i].second));
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
    const GLint maxClipCull = GLUtil::getGLValue(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES);

    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#version 4%d0 core", minGLVersion), lineOffsets);

    appendToShaderHeader(ShaderType::COUNT, "/*Copyright 2009-2020 DIVIDE-Studio*/", lineOffsets);
    if (getStateTracker()._opengl46Supported) {
        appendToShaderHeader(ShaderType::COUNT, "#define OPENGL_46", lineOffsets);
    } else {
        appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_shader_draw_parameters : require", lineOffsets);
    }
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_cull_distance : require", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_gpu_shader5 : require", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, "#extension GL_ARB_enhanced_layouts : require", lineOffsets);
    
    appendToShaderHeader(ShaderType::COUNT, crossTypeGLSLHLSL, lineOffsets);

    // Add current build environment information to the shaders
    if_constexpr(Config::Build::IS_DEBUG_BUILD) {
        appendToShaderHeader(ShaderType::COUNT, "#define _DEBUG", lineOffsets);
    } else if_constexpr(Config::Build::IS_PROFILE_BUILD) {
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
        if_constexpr(Config::ENABLE_GPU_VALIDATION) {
            appendToShaderHeader(ShaderType::COUNT, "#pragma option strict on", lineOffsets);
        }
        appendToShaderHeader(ShaderType::COUNT, "//#pragma option unroll all", lineOffsets);
    }

    if_constexpr(Config::USE_COLOURED_WOIT) {
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
        ShaderType::COMPUTE,
        "#define BUFFER_LUMINANCE_HISTOGRAM " +
        to_stringImpl(to_base(ShaderBufferLocation::LUMINANCE_HISTOGRAM)),
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
        to_stringImpl(to_base(TextureUsage::UNIT0)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_HEIGHT " +
        to_stringImpl(to_base(TextureUsage::HEIGHTMAP)),
        lineOffsets);
    
    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_UNIT1 " +
        to_stringImpl(to_base(TextureUsage::UNIT1)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COMPUTE,
        "#define TEXTURE_UNIT1 " +
        to_stringImpl(to_base(TextureUsage::UNIT1)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_NORMALMAP " +
        to_stringImpl(to_base(TextureUsage::NORMALMAP)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_OPACITY " +
        to_stringImpl(to_base(TextureUsage::OPACITY)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_SPECULAR " +
        to_stringImpl(to_base(TextureUsage::SPECULAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_GLOSS " +
        to_stringImpl(to_base(TextureUsage::GLOSS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_ROUGHNESS " +
        to_stringImpl(to_base(TextureUsage::ROUGHNESS)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_PROJECTION " +
        to_stringImpl(to_base(TextureUsage::PROJECTION)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_DEPTH_MAP " +
        to_stringImpl(to_base(TextureUsage::DEPTH)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_DEPTH_MAP_PREV " +
        to_stringImpl(to_base(TextureUsage::DEPTH_PREV)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFLECTION_PLANAR " +
        to_stringImpl(to_base(TextureUsage::REFLECTION_PLANAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFRACTION_PLANAR " +
        to_stringImpl(to_base(TextureUsage::REFRACTION_PLANAR)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_REFLECTION_CUBE " +
        to_stringImpl(to_base(TextureUsage::REFLECTION_CUBE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_GBUFFER_EXTRA " +
        to_stringImpl(to_base(TextureUsage::GBUFFER_EXTRA)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_CUBE_MAP_ARRAY " +
        to_stringImpl(to_base(TextureUsage::SHADOW_CUBE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_LAYERED_MAP_ARRAY " +
        to_stringImpl(to_U32(TextureUsage::SHADOW_LAYERED)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_SINGLE_MAP_ARRAY " +
        to_stringImpl(to_U32(TextureUsage::SHADOW_SINGLE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_COUNT " +
        to_stringImpl(to_U32(TextureUsage::COUNT)),
        lineOffsets);

    if_constexpr(Config::Lighting::USE_SEPARATE_VSM_PASS) {
        appendToShaderHeader(
            ShaderType::FRAGMENT,
            "#define USE_SEPARATE_VSM_PASS",
            lineOffsets);
    }

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

    const auto addVaryings = [&](ShaderType type, ShaderOffsetArray& lineOffsets, bool bump) {
        if (bump) {
            for (U8 i = 0; i < (sizeof(shaderVaryingsBump) / sizeof(shaderVaryingsBump[0])); ++i) {
                appendToShaderHeader(type, Util::StringFormat("    %s %s;", shaderVaryingsBump[i].first, shaderVaryingsBump[i].second), lineOffsets);
            }
        } else {
            for (U8 i = 0; i < (sizeof(shaderVaryings) / sizeof(shaderVaryings[0])); ++i) {
                appendToShaderHeader(type, Util::StringFormat("    %s %s;", shaderVaryings[i].first, shaderVaryings[i].second), lineOffsets);
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

bool GL_API::deInitGLSW() {
    // Shutdown GLSW
    return glswShutdown() == 1;
}
/// Try to find the requested font in the font cache. Load on cache miss.
I32 GL_API::getFont(const Str64& fontName) {
    if (_fontCache.first.compare(fontName) != 0) {
        _fontCache.first = fontName;
        const U64 fontNameHash = _ID(fontName.c_str());
        // Search for the requested font by name
        const FontCache::const_iterator it = _fonts.find(fontNameHash);
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
    OPTICK_EVENT();

    pushDebugMessage("OpenGL render text start!", 2);

    getStateTracker().setBlending(0,
        BlendingProperties{
            BlendProperty::SRC_ALPHA,
            BlendProperty::INV_SRC_ALPHA,
            BlendOperation::ADD,

            BlendProperty::ONE,
            BlendProperty::ZERO,
            BlendOperation::COUNT,

            true //enabled
        }
    );
    getStateTracker().setBlendColour(DefaultColours::DIVIDE_BLUE_U8);

    const I32 width = _context.renderingResolution().width;
    const I32 height = _context.renderingResolution().height;
        
    vec_size drawCount = 0;
    size_t previousStyle = 0;

    fonsClearState(_fonsContext);
    for (const TextElement& entry : batch())
    {
        if (previousStyle != entry._textLabelStyleHash) {
            const TextLabelStyle& textLabelStyle = TextLabelStyle::get(entry._textLabelStyleHash);
            const UColour4& colour = textLabelStyle.colour();
            // Retrieve the font from the font cache
            const I32 font = getFont(TextLabelStyle::fontName(textLabelStyle.font()));
            // The font may be invalid, so skip this text label
            if (font != FONS_INVALID) {
                fonsSetFont(_fonsContext, font);
            }
            fonsSetBlur(_fonsContext, textLabelStyle.blurAmount());
            fonsSetBlur(_fonsContext, textLabelStyle.spacing());
            fonsSetAlign(_fonsContext, textLabelStyle.alignFlag());
            fonsSetSize(_fonsContext, to_F32(textLabelStyle.fontSize()));
            fonsSetColour(_fonsContext, colour.r, colour.g, colour.b, colour.a);
            previousStyle = entry._textLabelStyleHash;
        }

        const F32 textX = entry._position.d_x.d_scale * width + entry._position.d_x.d_offset;
        const F32 textY = height - (entry._position.d_y.d_scale * height + entry._position.d_y.d_offset);

        F32 lh = 0;
        fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);
        
        const TextElement::TextType& text = entry.text();
        const vec_size lineCount = text.size();
        for (vec_size i = 0; i < lineCount; ++i) {
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
    OPTICK_EVENT();

    if (data != nullptr && data->Valid) {

        GenericVertexData::IndexBuffer idxBuffer;
        GenericDrawCommand cmd = {};
        cmd._primitiveType = PrimitiveType::TRIANGLES;

        const ImVec2 pos = data->DisplayPos;
        for (int n = 0; n < data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = data->CmdLists[n];
            const U32 vertCount = to_U32(cmd_list->VtxBuffer.size());
            assert(vertCount < MAX_IMGUI_VERTS);

            cmd._cmd.firstIndex = 0;

            idxBuffer.smallIndices = sizeof(ImDrawIdx) == 2;
            idxBuffer.count = to_U32(cmd_list->IdxBuffer.Size);
            idxBuffer.data = (Byte*)cmd_list->IdxBuffer.Data;

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
                    const Rect<I32>& viewport = GL_API::getStateTracker()._activeViewport;
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

                        GL_API::getStateTracker().bindTexture(0, TextureType::TEXTURE_2D, (GLuint)((intptr_t)pcmd->TextureId));
                        buffer->draw(cmd, 0);
                    }
                }
                cmd._cmd.firstIndex += pcmd->ElemCount;
            }
        }
    }
}

bool GL_API::bindPipeline(const Pipeline& pipeline, bool& shaderWasReady) {
    OPTICK_EVENT();

    if (GL_API::getStateTracker()._activePipeline && *GL_API::getStateTracker()._activePipeline == pipeline) {
        return true;
    }

    GL_API::getStateTracker()._activePipeline = &pipeline;

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
    if (Attorney::GLAPIShaderProgram::bind(glProgram, wasBound, shaderWasReady)) {
        const ShaderFunctions& functions = pipeline.shaderFunctions();
        for (U8 type = 0; type < to_U8(ShaderType::COUNT); ++type) {
            Attorney::GLAPIShaderProgram::SetSubroutines(glProgram, static_cast<ShaderType>(type), functions[type]);
        }
        if (!wasBound) {
            Attorney::GLAPIShaderProgram::queueValidation(glProgram);
        }
        return true;
    }

    GL_API::getStateTracker().setActiveProgram(0u);
    GL_API::getStateTracker().setActivePipeline(0u);
    GL_API::getStateTracker()._activePipeline = nullptr;

    return false;
}

void GL_API::sendPushConstants(const PushConstants& pushConstants) {
    OPTICK_EVENT();

    assert(GL_API::getStateTracker()._activePipeline != nullptr);

    ShaderProgram* program = ShaderProgram::findShaderProgram(GL_API::getStateTracker()._activePipeline->shaderProgramHandle());
    if (program == nullptr) {
        // Should we skip the upload?
        program = ShaderProgram::defaultShader().get();
    }

    static_cast<glShaderProgram*>(program)->UploadPushConstants(pushConstants);
}

bool GL_API::draw(const GenericDrawCommand& cmd, U32 cmdBufferOffset) {
    OPTICK_EVENT();

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
    lockBuffers(_context.getFrameCount());

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

void GL_API::flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) {
    OPTICK_EVENT();

    const GFX::CommandType cmdType = static_cast<GFX::CommandType>(entry._typeIndex);

    OPTICK_TAG("Type", to_base(cmdType));

    switch (cmdType) {
        case GFX::CommandType::BEGIN_RENDER_PASS: {
            GFX::BeginRenderPassCommand* crtCmd = commandBuffer.get<GFX::BeginRenderPassCommand>(entry);
            glFramebuffer& rt = static_cast<glFramebuffer&>(_context.renderTargetPool().renderTarget(crtCmd->_target));
            Attorney::GLAPIRenderTarget::begin(rt, crtCmd->_descriptor);
            GL_API::getStateTracker()._activeRenderTarget = &rt;
            GL_API::pushDebugMessage(crtCmd->_name.c_str(), std::numeric_limits<I32>::max());
        }break;
        case GFX::CommandType::END_RENDER_PASS: {
            GFX::EndRenderPassCommand* crtCmd = commandBuffer.get<GFX::EndRenderPassCommand>(entry);
            assert(GL_API::getStateTracker()._activeRenderTarget != nullptr);
            GL_API::popDebugMessage();
            glFramebuffer& fb = *GL_API::getStateTracker()._activeRenderTarget;
            Attorney::GLAPIRenderTarget::end(fb, crtCmd->_setDefaultRTState);
        }break;
        case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
            GFX::BeginPixelBufferCommand* crtCmd = commandBuffer.get<GFX::BeginPixelBufferCommand>(entry);
            assert(crtCmd->_buffer != nullptr);
            glPixelBuffer* buffer = static_cast<glPixelBuffer*>(crtCmd->_buffer);
            bufferPtr data = Attorney::GLAPIPixelBuffer::begin(*buffer);
            if (crtCmd->_command) {
                crtCmd->_command(data);
            }
            GL_API::getStateTracker()._activePixelBuffer = buffer;
        }break;
        case GFX::CommandType::END_PIXEL_BUFFER: {
            assert(GL_API::getStateTracker()._activePixelBuffer != nullptr);
            Attorney::GLAPIPixelBuffer::end(*GL_API::getStateTracker()._activePixelBuffer);
        }break;
        case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
            assert(GL_API::getStateTracker()._activeRenderTarget != nullptr);
            GFX::BeginRenderSubPassCommand* crtCmd = commandBuffer.get<GFX::BeginRenderSubPassCommand>(entry);
            for (const RenderTarget::DrawLayerParams& params : crtCmd->_writeLayers) {
                GL_API::getStateTracker()._activeRenderTarget->drawToLayer(params);
            }

            GL_API::getStateTracker()._activeRenderTarget->setMipLevel(crtCmd->_mipWriteLevel, crtCmd->_validateWriteLevel);
        }break;
        case GFX::CommandType::END_RENDER_SUB_PASS: {
        }break;
        case GFX::CommandType::COPY_TEXTURE: {
            GFX::CopyTextureCommand* crtCmd = commandBuffer.get<GFX::CopyTextureCommand>(entry);
            glTexture::copy(crtCmd->_source, crtCmd->_destination, crtCmd->_params);
        }break;
        case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
            GFX::BindDescriptorSetsCommand* crtCmd = commandBuffer.get<GFX::BindDescriptorSetsCommand>(entry);
            const DescriptorSet& set = crtCmd->_set;

            if (!makeTexturesResident(set._textureData, set._textureViews)) {
                //Error
            }
            if (!makeImagesResident(set._images)) {
                //Error
            }
            {
                OPTICK_EVENT("Bind Shader Buffers");
                for (const ShaderBufferBinding& shaderBufCmd : set._shaderBuffers) {
                    glUniformBuffer* buffer = static_cast<glUniformBuffer*>(shaderBufCmd._buffer);
                    buffer->bindRange(to_U8(shaderBufCmd._binding), shaderBufCmd._elementRange.x, shaderBufCmd._elementRange.y);
                }
            }
        }break;
        case GFX::CommandType::BIND_PIPELINE: {
            const Pipeline* pipeline = commandBuffer.get<GFX::BindPipelineCommand>(entry)->_pipeline;
            assert(pipeline != nullptr);
            bool shaderWasReady = false;
            if (!bindPipeline(*pipeline, shaderWasReady) && shaderWasReady) {
                Console::errorfn(Locale::get(_ID("ERROR_GLSL_INVALID_BIND")), pipeline->shaderProgramHandle());
            }
        } break;
        case GFX::CommandType::SEND_PUSH_CONSTANTS: {
            if (GL_API::getStateTracker()._activePipeline != nullptr) {
                sendPushConstants(commandBuffer.get<GFX::SendPushConstantsCommand>(entry)->_constants);
            }
        } break;
        case GFX::CommandType::SET_SCISSOR: {
            getStateTracker().setScissor(commandBuffer.get<GFX::SetScissorCommand>(entry)->_rect);
        }break;
        case GFX::CommandType::SET_BLEND: {
            GFX::SetBlendCommand* blendCmd = commandBuffer.get<GFX::SetBlendCommand>(entry);
            getStateTracker().setBlending(blendCmd->_blendProperties);
        }break;
        case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
             GFX::BeginDebugScopeCommand* crtCmd = commandBuffer.get<GFX::BeginDebugScopeCommand>(entry);
             pushDebugMessage(crtCmd->_scopeName.c_str(), crtCmd->_scopeID);
        } break;
        case GFX::CommandType::END_DEBUG_SCOPE: {
            popDebugMessage();
        } break;
        case GFX::CommandType::COMPUTE_MIPMAPS: {
            OPTICK_EVENT("GL: Compute MipMaps");
            GFX::ComputeMipMapsCommand* crtCmd = commandBuffer.get<GFX::ComputeMipMapsCommand>(entry);
            if (crtCmd->_layerRange.x == 0 && crtCmd->_layerRange.y <= 1) {
                glGenerateTextureMipmap(crtCmd->_texture->data()._textureHandle);
            } else {
                TextureView view = {};
                view._texture = crtCmd->_texture;
                view._layerRange.set(crtCmd->_layerRange.x, crtCmd->_layerRange.y);
                view._mipLevels.set(view._texture->descriptor().mipLevels());

                const TextureDescriptor& descriptor = view._texture->descriptor();
                const GLenum glInternalFormat = GLUtil::internalFormat(descriptor.baseFormat(), descriptor.dataType(), descriptor.srgb());

                const TextureData& data = view._texture->data();
                assert(IsValid(data));

                const GLenum type = GLUtil::glTextureTypeTable[to_base(data._textureType)];

                std::pair<GLuint, bool> handle = s_texturePool.allocate(view.getHash(), GL_NONE);
                if (!handle.second) {
                    glTextureView(handle.first,
                        type,
                        data._textureHandle,
                        glInternalFormat,
                        (GLuint)view._mipLevels.x,
                        (GLuint)view._mipLevels.y,
                        (GLuint)view._layerRange.x,
                        (GLuint)view._layerRange.y);
                }

                glGenerateTextureMipmap(handle.first);

                s_texturePool.deallocate(handle.first, GL_NONE, 3);
            }
        }break;
        case GFX::CommandType::DRAW_TEXT: {
            if (GL_API::getStateTracker()._activePipeline != nullptr) {
                GFX::DrawTextCommand* crtCmd = commandBuffer.get<GFX::DrawTextCommand>(entry);
                drawText(crtCmd->_batch);
                lockBuffers(_context.getFrameCount());
            }
        }break;
        case GFX::CommandType::DRAW_IMGUI: {
            if (GL_API::getStateTracker()._activePipeline != nullptr) {
                GFX::DrawIMGUICommand* crtCmd = commandBuffer.get<GFX::DrawIMGUICommand>(entry);
                drawIMGUI(crtCmd->_data, crtCmd->_windowGUID);
                lockBuffers(_context.getFrameCount());
            }
        }break;
        case GFX::CommandType::DRAW_COMMANDS : {
            const GLStateTracker& stateTracker = GL_API::getStateTracker();
            if (stateTracker._activePipeline != nullptr) {
                GFX::DrawCommand* crtCmd = commandBuffer.get<GFX::DrawCommand>(entry);
                const GFX::DrawCommand::CommandContainer& drawCommands = crtCmd->_drawCommands;
                for (const GenericDrawCommand& currentDrawCommand : drawCommands) {
                    if (draw(currentDrawCommand, stateTracker._commandBufferOffset)) {
                        if (isEnabledOption(currentDrawCommand, CmdRenderOptions::RENDER_GEOMETRY)) {
                            if (isEnabledOption(currentDrawCommand, CmdRenderOptions::RENDER_WIREFRAME)) {
                                _context.registerDrawCalls(2);
                            } else {
                                _context.registerDrawCall();
                            }
                        }
                    }
                }
            }
        }break;
        case GFX::CommandType::DISPATCH_COMPUTE: {
            GFX::DispatchComputeCommand* crtCmd = commandBuffer.get<GFX::DispatchComputeCommand>(entry);
            if(GL_API::getStateTracker()._activePipeline != nullptr) {
                OPTICK_EVENT("GL: Dispatch Compute");
                glDispatchCompute(crtCmd->_computeGroupSize.x, crtCmd->_computeGroupSize.y, crtCmd->_computeGroupSize.z);
                lockBuffers(_context.getFrameCount());
            }
        }break;
        case GFX::CommandType::SET_CLIPING_STATE: {
            GFX::SetClippingStateCommand* crtCmd = commandBuffer.get<GFX::SetClippingStateCommand>(entry);
            getStateTracker().setClipingPlaneState(crtCmd->_lowerLeftOrigin, crtCmd->_negativeOneToOneDepth);
        } break;
        case GFX::CommandType::MEMORY_BARRIER: {
            GFX::MemoryBarrierCommand* crtCmd = commandBuffer.get<GFX::MemoryBarrierCommand>(entry);
            MemoryBarrierMask glMask = MemoryBarrierMask::GL_NONE_BIT;
            const U32 barrierMask = crtCmd->_barrierMask;
            if (barrierMask != 0) {
                if (BitCompare(barrierMask, to_base(MemoryBarrierType::TEXTURE_BARRIER))) {
                    glTextureBarrier();
                } 
                if (barrierMask == to_base(MemoryBarrierType::ALL_MEM_BARRIERS)) {
                    glMemoryBarrier(MemoryBarrierMask::GL_ALL_BARRIER_BITS);
                } else {
                    for (U8 i = 0; i < to_U8(MemoryBarrierType::COUNT) + 1; ++i) {
                        if (BitCompare(barrierMask, to_U32(1 << i))) {
                            switch (static_cast<MemoryBarrierType>(1 << i)) {
                                case MemoryBarrierType::BUFFER_UPDATE:
                                    glMask |= MemoryBarrierMask::GL_BUFFER_UPDATE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::SHADER_STORAGE:
                                    glMask |= MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::COMMAND_BUFFER:
                                    glMask |= MemoryBarrierMask::GL_COMMAND_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::ATOMIC_COUNTER:
                                    glMask |= MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::QUERY:
                                    glMask |= MemoryBarrierMask::GL_QUERY_BUFFER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::RENDER_TARGET:
                                    glMask |= MemoryBarrierMask::GL_FRAMEBUFFER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::TEXTURE_UPDATE:
                                    glMask |= MemoryBarrierMask::GL_TEXTURE_UPDATE_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::TEXTURE_FETCH:
                                    glMask |= MemoryBarrierMask::GL_TEXTURE_FETCH_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::SHADER_IMAGE:
                                    glMask |= MemoryBarrierMask::GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::TRANSFORM_FEEDBACK:
                                    glMask |= MemoryBarrierMask::GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::VERTEX_ATTRIB_ARRAY:
                                    glMask |= MemoryBarrierMask::GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::INDEX_ARRAY:
                                    glMask |= MemoryBarrierMask::GL_ELEMENT_ARRAY_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::UNIFORM_DATA:
                                    glMask |= MemoryBarrierMask::GL_UNIFORM_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::PIXEL_BUFFER:
                                    glMask |= MemoryBarrierMask::GL_PIXEL_BUFFER_BARRIER_BIT;
                                    break;
                                case MemoryBarrierType::PERSISTENT_BUFFER:
                                    glMask |= MemoryBarrierMask::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
                                    break;
                                default:
                                    NOP();
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

void GL_API::lockBuffers(U32 frameID) {
    OPTICK_EVENT();
    for (const BufferLockEntry& entry : s_bufferLockQueue) {
        entry._buffer->lockRange(entry._offset, entry._length, frameID);
        s_glFlushQueued = s_glFlushQueued || entry._flush;
    }
    s_bufferLockQueue.resize(0);
}

void GL_API::registerBufferBind(BufferLockEntry&& data) {
    assert(Runtime::isMainThread());
    s_bufferLockQueue.push_back(data);
}

void GL_API::preFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    ACKNOWLEDGE_UNUSED(commandBuffer);
}

void GL_API::postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer) {
    ACKNOWLEDGE_UNUSED(commandBuffer);
    if (s_glFlushQueued) {
        OPTICK_EVENT("GL_FLUSH");
        glFlush();
        s_glFlushQueued = false;
    }
}

GenericVertexData* GL_API::getOrCreateIMGUIBuffer(I64 windowGUID) {
    GenericVertexData* ret = nullptr;

    const auto it = _IMGUIBuffers.find(windowGUID);
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
    OPTICK_EVENT();

    // Passing 0 is a perfectly acceptable way of enabling the default render state block
    if (stateBlockHash == 0) {
        stateBlockHash = _context.getDefaultStateBlock(false);
    }

    return getStateTracker().setStateBlock(stateBlockHash);
}

bool GL_API::makeImagesResident(const vectorEASTLFast<Image>& images) {
    OPTICK_EVENT();

    for (const Image& image : images) {
        if (image._texture != nullptr) {
            image._texture->bindLayer(image._binding, image._level, image._layer, false, image._flag != Image::Flag::WRITE ? true : false, image._flag != Image::Flag::READ ? true : false);
        }
    }

    return true;
}

bool GL_API::makeTexturesResident(const TextureDataContainer<>& textureData, const vectorEASTLFast<TextureViewEntry>& textureViews) {
    OPTICK_EVENT();

    bool bound = false;

    STUBBED("ToDo: Optimise this: If over n textures, get max binding slot, create [0...maxSlot] bindings, fill unused with 0 and send as one command with glBindTextures -Ionut")
    constexpr vec_size k_textureThreshold = 3;
    const U8 texCount = textureData.count();
    if (texCount > k_textureThreshold && false) {
        constexpr GLushort offset = 0;
        vectorEASTL<TextureType> types;
        vectorEASTL<GLuint> handles;
        vectorEASTL<GLuint> samplers;
        
        types.reserve(texCount);
        handles.reserve(texCount);
        samplers.reserve(texCount);

        for (const auto& [binding, data] : textureData.textures()) {
            if (binding == TextureDataContainer<>::INVALID_BINDING) {
                continue;
            }
            assert(IsValid(data));
            types.push_back(data._textureType);
            handles.push_back(data._textureHandle);
            samplers.push_back(data._samplerHandle);
        }

        bound = getStateTracker().bindTextures(offset, (GLuint)texCount, types.data(), handles.data(), samplers.data());
    } else {
        GLStateTracker& stateTracker = getStateTracker();
        for (const auto&[binding, data] : textureData.textures()) {
            if (binding == TextureDataContainer<>::INVALID_BINDING) {
                continue;
            }
            assert(IsValid(data));
            bound = stateTracker.bindTexture(static_cast<GLushort>(binding),
                                             data._textureType,
                                             data._textureHandle,
                                             data._samplerHandle) || bound;
        }
    }

    for (auto it : textureViews) {
        const size_t viewHash = it.getHash();

        std::pair<GLuint,bool> handle = s_texturePool.allocate(viewHash, GL_NONE);
        if (handle.first == 0u) {
            DIVIDE_UNEXPECTED_CALL();
            continue;
        }

        const Texture* tex = it._view._texture;
        assert(tex != nullptr);
        const TextureData& data = tex->data();
        assert(IsValid(data));

        if (!handle.second) {
            const TextureDescriptor& descriptor = tex->descriptor();
            const GLenum type = GLUtil::glTextureTypeTable[to_base(data._textureType)];
            const GLenum glInternalFormat = GLUtil::internalFormat(descriptor.baseFormat(), descriptor.dataType(), descriptor.srgb());

            glTextureView(handle.first,
                type,
                data._textureHandle,
                glInternalFormat,
                (GLuint)it._view._mipLevels.x,
                (GLuint)it._view._mipLevels.y,
                (GLuint)it._view._layerRange.x,
                (GLuint)it._view._layerRange.y);
        }
        bound = getStateTracker().bindTexture(static_cast<GLushort>(it._binding), data._textureType, handle.first, data._samplerHandle) || bound;
        // Self delete after 3 frames unless we use it again
        s_texturePool.deallocate(handle.first, GL_NONE, 3u);
    }

    return bound;
}

bool GL_API::setViewport(const Rect<I32>& viewport) {
    OPTICK_EVENT();

    return getStateTracker().setViewport(viewport);
}

/// Verify if we have a sampler object created and available for the given descriptor
U32 GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor) {
    OPTICK_EVENT();

    // Get the descriptor's hash value
    const size_t hashValue = descriptor.getHash();
    // Try to find the hash value in the sampler object map
    GLuint sampler = getSamplerHandle(hashValue);
    if (sampler == 0) {
        UniqueLock<Mutex> w_lock(s_samplerMapLock);
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
        UniqueLock<Mutex> r_lock(s_samplerMapLock);
        const SamplerObjectMap::const_iterator it = s_samplerMap.find(samplerHash);
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

IMPrimitive* GL_API::newIMP(Mutex& lock, GFXDevice& parent) {
    return s_IMPrimitivePool.newElement(lock, parent);
}

bool GL_API::destroyIMP(Mutex& lock, IMPrimitive*& primitive) {
    if (primitive != nullptr) {
        s_IMPrimitivePool.deleteElement(lock, static_cast<glIMPrimitive*>(primitive));
        primitive = nullptr;
        return true;
    }

    return false;
}

};
