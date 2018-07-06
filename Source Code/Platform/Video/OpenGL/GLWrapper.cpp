#include "stdafx.h"

#include "Headers/GLWrapper.h"
#include "Headers/glIMPrimitive.h"
#include "Headers/glHardwareQuery.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

#include "GUI/Headers/GUIText.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
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

#if !defined(CEGUI_STATIC)
#define CEGUI_STATIC
#endif
#include <CEGUI/CEGUI.h>
#include <GL3Renderer.h>
#include <Texture.h>

namespace Divide {

namespace {
    /// How many frames of delay do we allow between the current read and write queries
    /// The longer the delay, the better the performance but the more out of sync the results
    U32 g_performanceQueryRingLength = 2;
    U32 g_performanceQueryRingLengthMax = 6;

    // If this is true, every time a query result isn't available, we increase the 
    // the query buffer ring length by 1 up to a maximum of g_performanceQueryRingLengthMax
    bool g_autoAdjustQueryLength = true;
};

GL_API::GL_API(GFXDevice& context)
    : RenderAPIWrapper(),
      _context(context),
      _prevSizeNode(0),
      _prevSizeString(0),
      _prevWidthNode(0),
      _prevWidthString(0),
      _fonsContext(nullptr),
      _GUIGLrenderer(nullptr),
#if !defined(USE_FIXED_FUNCTION_IMGUI)
      _IMGUIBuffer(nullptr),
#endif
      _swapBufferTimer(Time::ADD_TIMER("Swap Buffer Timer"))
{
    // Only updated in Debug builds
    FRAME_DURATION_GPU = 0u;
    // All clip planes are disabled at first (default OpenGL state)
    _activeClipPlanes.fill(false);
    _fontCache.second = -1;
    s_samplerBoundMap.fill(0u);
    s_textureBoundMap.fill(0u);

    _elapsedTimeQuery = std::make_shared<glHardwareQueryRing>(context, g_performanceQueryRingLength);
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

/// Prepare the GPU for rendering a frame
void GL_API::beginFrame() {
    // Start a duration query in debug builds
    if (Config::ENABLE_GPU_VALIDATION) {
        GLuint writeQuery = _elapsedTimeQuery->writeQuery().getID();
        glBeginQuery(GL_TIME_ELAPSED, writeQuery);
    }

    // Clear our buffers
    const WindowManager& winMgr = _context.parent().platformContext().app().windowManager();
    U32 windowCount = winMgr.getWindowCount();
    for (U32 i = 0; i < windowCount; ++i) {
        const DisplayWindow& window = winMgr.getWindow(i);
        if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
            GL_API::setClearColour(window.clearColour());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT /* | GL_STENCIL_BUFFER_BIT*/);
        }
    }
    
    
    // Clears are registered as draw calls by most software, so we do the same
    // to stay in sync with third party software
    _context.registerDrawCall();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    s_previousStateBlockHash = 0;
}

/// Finish rendering the current frame
void GL_API::endFrame() {
    // Revert back to the default OpenGL states
    clearStates();
    // Swap buffers
    {
        Time::ScopedTimer time(_swapBufferTimer);
        const WindowManager& winMgr = _context.parent().platformContext().app().windowManager();
        U32 windowCount = winMgr.getWindowCount();
        for (U32 i = 0; i < windowCount; ++i) {
            const DisplayWindow& window = winMgr.getWindow(i);
            if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
                SDL_GL_SwapWindow(window.getRawWindow());
            }
        }
    }
    // End the timing query started in beginFrame() in debug builds
    if (Config::ENABLE_GPU_VALIDATION) {
        glEndQuery(GL_TIME_ELAPSED);
        _elapsedTimeQuery->incQueue();
    }
}

U32 GL_API::getFrameDurationGPU() {
    if (Config::ENABLE_GPU_VALIDATION) {
        // The returned results are 'g_performanceQueryRingLength - 1' frames old!
        GLuint readQuery = _elapsedTimeQuery->readQuery().getID();
        GLint available = 0;
        glGetQueryObjectiv(readQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        // See how much time the rendering of object i took in nanoseconds.
        if (!available) {
            if (g_autoAdjustQueryLength) {
                U32 newQueryRingLength = std::min(g_performanceQueryRingLength + 1,
                                                  g_performanceQueryRingLengthMax);
                if (newQueryRingLength != g_performanceQueryRingLength) {
                    g_performanceQueryRingLength = newQueryRingLength;
                    _elapsedTimeQuery->resize(g_performanceQueryRingLength);
                }
            }
        } else {
            glGetQueryObjectuiv(readQuery,
                                GL_QUERY_RESULT,
                                &FRAME_DURATION_GPU);
            FRAME_DURATION_GPU = Time::NanosecondsToMilliseconds<U32>(FRAME_DURATION_GPU);
        }
    }

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

        case ShaderType::TESSELATION_CTRL:
            stage = "TessellationC";
            break;

        case ShaderType::TESSELATION_EVAL:
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
    if (entry.find("#include") == std::string::npos) {
        inOutOffset[index] += Util::LineCount(entry);
    } else {
        vectorImpl<stringImpl> tempAtoms;
        inOutOffset[index] += Util::LineCount(glShader::preprocessIncludes("header", entry, 0, tempAtoms));
    }
}

/// Prepare our shader loading system
bool GL_API::initShaders() {

    
    static const std::pair<stringImpl, stringImpl>  shaderVaryings[] =
    { {"flat uint" , "dvd_instanceID"},
      {"flat uint" , "dvd_drawID"},
      { "vec2" , "_texCoord"},
      { "vec4" , "_vertexW"},
      { "vec4" , "_vertexWV"},
      { "vec3" , "_normalWV"},
      { "vec3" , "_tangentWV"},
      { "vec3" , "_bitangentWV"}
    };

    static const stringImpl crossTypeGLSLHLSL = "#define float2 vec2\n"
                                                "#define float3 vec3\n"
                                                "#define float4 vec4\n"
                                                "#define int2 ivec2\n"
                                                "#define int3 ivec3\n"
                                                "#define int4 ivec4\n"
                                                "#define float2x2 mat2\n"
                                                "#define float3x3 mat3\n"
                                                "#define float4x4 mat4";

    auto getPassData = [](ShaderType type) -> stringImpl {
        stringImpl baseString = "     _out.%s = _in[index].%s;";
        if (type == ShaderType::TESSELATION_CTRL) {
            baseString = "    _out[gl_InvocationID].%s = _in[index].%s;";
        }

        stringImpl passData("void PassData(in int index) {");
        passData.append("\n");
        for (const std::pair<stringImpl, stringImpl>& var : shaderVaryings) {
            passData.append(Util::StringFormat(baseString.c_str(), var.second.c_str(), var.second.c_str()));
            passData.append("\n");
        }
        passData.append("}\n");

        return passData;
    };

    // Initialize GLSW
    GLint glswState = glswInit();

    glShaderProgram::initStaticData();

    ShaderOffsetArray lineOffsets = {0};
  
    // Add our engine specific defines and various code pieces to every GLSL shader
    // Add version as the first shader statement, followed by copyright notice
    appendToShaderHeader(ShaderType::COUNT, "#version 450 core", lineOffsets);

    appendToShaderHeader(ShaderType::COUNT,
                         "/*Copyright 2009-2016 DIVIDE-Studio*/", lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         "#extension GL_ARB_shader_draw_parameters : require",
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         "#extension GL_ARB_gpu_shader5 : require",
                         lineOffsets);
    appendToShaderHeader(ShaderType::VERTEX, "invariant gl_Position;", lineOffsets);

    appendToShaderHeader(ShaderType::COUNT,
                         crossTypeGLSLHLSL,
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         Util::StringFormat("#define GPU_VENDOR_AMD %d", to_base(GPUVendor::AMD)),
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         Util::StringFormat("#define GPU_VENDOR_NVIDIA %d", to_base(GPUVendor::NVIDIA)),
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         Util::StringFormat("#define GPU_VENDOR_INTEL %d", to_base(GPUVendor::INTEL)),
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         Util::StringFormat("#define GPU_VENDOR_OTHER %d", to_base(GPUVendor::OTHER)),
                         lineOffsets);
    appendToShaderHeader(ShaderType::COUNT,
                         Util::StringFormat("#define GPU_VENDOR %d", to_U32(GFXDevice::getGPUVendor())),
                         lineOffsets);

    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define DETAIL_OFF   %d", to_base(RenderDetailLevel::OFF)),    lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define DETAIL_LOW   %d", to_base(RenderDetailLevel::LOW)),    lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define DETAIL_MED   %d", to_base(RenderDetailLevel::MEDIUM)), lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define DETAIL_HIGH  %d", to_base(RenderDetailLevel::HIGH)),   lineOffsets);
    appendToShaderHeader(ShaderType::COUNT, Util::StringFormat("#define DETAIL_ULTRA %d", to_base(RenderDetailLevel::ULTRA)),  lineOffsets);

    // Add current build environment information to the shaders
    if (Config::Build::IS_DEBUG_BUILD) {
        appendToShaderHeader(ShaderType::COUNT, "#define _DEBUG", lineOffsets);
    } else if(Config::Build::IS_PROFILE_BUILD) {
        appendToShaderHeader(ShaderType::COUNT, "#define _PROFILE", lineOffsets);
    } else {
        appendToShaderHeader(ShaderType::COUNT, "#define _RELEASE", lineOffsets);
    }

    // Shader stage level reflection system. A shader stage must know what stage
    // it's used for
    appendToShaderHeader(ShaderType::VERTEX, "#define VERT_SHADER",
                         lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "#define FRAG_SHADER",
                         lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "#define GEOM_SHADER",
                         lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_EVAL,
                         "#define TESS_EVAL_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_CTRL,
                         "#define TESS_CTRL_SHADER", lineOffsets);
    appendToShaderHeader(ShaderType::COMPUTE, "#define COMPUTE_SHADER",
                         lineOffsets);

    // This line gets replaced in every shader at load with the custom list of
    // defines specified by the material
    appendToShaderHeader(ShaderType::COUNT, "//__CUSTOM_DEFINES__",
                         lineOffsets);

    // Add some nVidia specific pragma directives
    if (GFXDevice::getGPUVendor() == GPUVendor::NVIDIA) {
        appendToShaderHeader(ShaderType::COUNT,
                             "//#pragma optionNV(fastmath on)", lineOffsets);
        appendToShaderHeader(ShaderType::COUNT,
                             "//#pragma optionNV(fastprecision on)",
                             lineOffsets);
        appendToShaderHeader(ShaderType::COUNT,
                             "//#pragma optionNV(inline all)", lineOffsets);
        appendToShaderHeader(ShaderType::COUNT,
                             "//#pragma optionNV(ifcvt none)", lineOffsets);
        appendToShaderHeader(ShaderType::COUNT, "//#pragma optionNV(strict on)",
                             lineOffsets);
        appendToShaderHeader(ShaderType::COUNT,
                             "//#pragma optionNV(unroll all)", lineOffsets);
    }

    if (Config::USE_HIZ_CULLING) {
        appendToShaderHeader(ShaderType::COUNT, "#define USE_HIZ_CULLING", lineOffsets);
    }

    if (Config::DEBUG_HIZ_CULLING) {
        appendToShaderHeader(ShaderType::COUNT, "#define DEBUG_HIZ_CULLING", lineOffsets);
    }

    appendToShaderHeader(
        ShaderType::COUNT,
        "const uint MAX_SPLITS_PER_LIGHT = " +
        to_stringImpl(Config::Lighting::MAX_SPLITS_PER_LIGHT) + ";",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "const uint MAX_POSSIBLE_LIGHTS = " +
        to_stringImpl(Config::Lighting::MAX_POSSIBLE_LIGHTS) + ";",
        lineOffsets);

    appendToShaderHeader(ShaderType::COUNT,
        "const int MAX_VISIBLE_NODES = " +
        to_stringImpl(Config::MAX_VISIBLE_NODES) + ";",
        lineOffsets);

    appendToShaderHeader(ShaderType::COUNT, "const float Z_TEST_SIGMA = 0.0001;", lineOffsets);

    appendToShaderHeader(ShaderType::COUNT, "const float ALPHA_DISCARD_THRESHOLD = 0.05;", lineOffsets);

    appendToShaderHeader(ShaderType::FRAGMENT, "const uint DEPTH_EXP_WARP = 32;", lineOffsets);

    // GLSL <-> VBO intercommunication
    appendToShaderHeader(ShaderType::VERTEX,
                        "const uint MAX_BONE_COUNT_PER_NODE = " +
                            to_stringImpl(Config::MAX_BONE_COUNT_PER_NODE) +
                        ";",
                        lineOffsets);
    if (Config::USE_HIZ_CULLING) {
        appendToShaderHeader(ShaderType::COUNT, "#define USE_HIZ_CULLING", lineOffsets);
    }
    if (Config::DEBUG_HIZ_CULLING) {
        appendToShaderHeader(ShaderType::COUNT, "#define DEBUG_HIZ_CULLING", lineOffsets);
    }

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_CLIP_PLANES " + to_stringImpl(to_base(Frustum::FrustPlane::COUNT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_SHADOW_CASTING_LIGHTS " +
        to_stringImpl(
        Config::Lighting::MAX_SHADOW_CASTING_LIGHTS),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_LIGHT_TYPES " +
        to_stringImpl(to_base(LightType::COUNT)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define BUFFER_GPU_BLOCK " +
        to_stringImpl(to_base(ShaderBufferLocation::GPU_BLOCK)),
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
        "#define BUFFER_PUSH_CONSTANTS " +
        to_stringImpl(to_base(ShaderBufferLocation::PUSH_CONSTANTS)),
        lineOffsets);
    
    appendToShaderHeader(
        ShaderType::COUNT,
        "#define FORWARD_PLUS_TILE_RES " + to_stringImpl(Config::Lighting::FORWARD_PLUS_TILE_RES),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define MAX_NUM_LIGHTS_PER_TILE " + to_stringImpl(Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define LIGHT_INDEX_BUFFER_SENTINEL " + to_stringImpl(Config::Lighting::FORWARD_PLUS_LIGHT_INDEX_BUFFER_SENTINEL),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define MAX_TEXTURE_SLOTS " + to_stringImpl(GL_API::s_maxTextureUnits),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_UNIT0 " +
            to_stringImpl(to_base(ShaderProgram::TextureUsage::UNIT0)),
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
        ShaderType::FRAGMENT,
        "#define TEXTURE_NORMALMAP " +
            to_stringImpl(to_base(ShaderProgram::TextureUsage::NORMALMAP)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::COUNT,
        "#define TEXTURE_OPACITY " +
            to_stringImpl(to_base(ShaderProgram::TextureUsage::OPACITY)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define TEXTURE_SPECULAR " +
            to_stringImpl(to_base(ShaderProgram::TextureUsage::SPECULAR)),
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
        "#define TEXTURE_REFRACTION_CUBE " +
            to_stringImpl(to_base(ShaderProgram::TextureUsage::REFRACTION_CUBE)),
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

    appendToShaderHeader(
        ShaderType::VERTEX,
        "#define DIRECTIONAL_LIGHT_DISTANCE_FACTOR " +
        to_stringImpl(to_base(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE)),
        lineOffsets);

    // Vertex data has a fixed format
    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_POSITION)) +
            ") in vec3 inVertexData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_COLOR)) +
            ") in vec4 inColourData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_NORMAL)) +
            ") in float inNormalData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_TEXCOORD)) +
            ") in vec2 inTexCoordData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_TANGENT)) +
            ") in float inTangentData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_BONE_WEIGHT)) +
            ") in vec4 inBoneWeightData;",
        lineOffsets);

    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_BONE_INDICE)) +
            ") in uvec4 inBoneIndiceData;",
        lineOffsets);
    
    appendToShaderHeader(
        ShaderType::VERTEX,
        "layout(location = " +
            to_stringImpl(to_base(AttribLocation::VERTEX_WIDTH)) +
            ") in uint inLineWidthData;",
        lineOffsets);


    auto addVaryings = [&](ShaderType type, ShaderOffsetArray& lineOffsets) {
        for (const std::pair<stringImpl, stringImpl>& entry : shaderVaryings) {
            appendToShaderHeader(type, Util::StringFormat("    %s %s;", entry.first.c_str(), entry.second.c_str()), lineOffsets);
        }
    };

    // Vertex shader output
    appendToShaderHeader(ShaderType::VERTEX, "out Data {", lineOffsets);
        addVaryings(ShaderType::VERTEX, lineOffsets);
    appendToShaderHeader(ShaderType::VERTEX, "} _out;\n", lineOffsets);

    // Tessellation Control shader input
    appendToShaderHeader(ShaderType::TESSELATION_CTRL, "in Data {", lineOffsets);
        addVaryings(ShaderType::TESSELATION_CTRL, lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_CTRL, "} _in[];\n", lineOffsets);

    // Tessellation Control shader output
    appendToShaderHeader(ShaderType::TESSELATION_CTRL, "out Data {", lineOffsets);
        addVaryings(ShaderType::TESSELATION_CTRL, lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_CTRL, "} _out[];\n", lineOffsets);

    appendToShaderHeader(ShaderType::TESSELATION_CTRL, getPassData(ShaderType::TESSELATION_CTRL), lineOffsets);

    // Tessellation Eval shader input
    appendToShaderHeader(ShaderType::TESSELATION_EVAL, "in Data {", lineOffsets);
        addVaryings(ShaderType::TESSELATION_EVAL, lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_EVAL, "} _in[];\n", lineOffsets);

    // Tessellation Eval shader output
    appendToShaderHeader(ShaderType::TESSELATION_EVAL, "out Data {", lineOffsets);
        addVaryings(ShaderType::TESSELATION_EVAL, lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_EVAL, "} _out;\n", lineOffsets);

    appendToShaderHeader(ShaderType::TESSELATION_EVAL, getPassData(ShaderType::TESSELATION_EVAL), lineOffsets);

    // Geometry shader input
    appendToShaderHeader(ShaderType::GEOMETRY, "in Data {", lineOffsets);
        addVaryings(ShaderType::GEOMETRY, lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "} _in[];\n", lineOffsets);

    // Geometry shader output
    appendToShaderHeader(ShaderType::GEOMETRY, "out Data {", lineOffsets);
        addVaryings(ShaderType::GEOMETRY, lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "} _out;\n", lineOffsets);

    appendToShaderHeader(ShaderType::GEOMETRY, getPassData(ShaderType::GEOMETRY), lineOffsets);

    // Fragment shader input
    appendToShaderHeader(ShaderType::FRAGMENT, "in Data {", lineOffsets);
        addVaryings(ShaderType::FRAGMENT, lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "} _in;\n", lineOffsets);
        
    appendToShaderHeader(ShaderType::VERTEX, "#define VAR _out", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_CTRL, "#define VAR _in[gl_InvocationID]", lineOffsets);
    appendToShaderHeader(ShaderType::TESSELATION_EVAL, "#define VAR _in", lineOffsets);
    appendToShaderHeader(ShaderType::GEOMETRY, "#define VAR _in", lineOffsets);
    appendToShaderHeader(ShaderType::FRAGMENT, "#define VAR _in", lineOffsets);

    // GPU specific data, such as GFXDevice's main uniform block and clipping
    // planes are defined in an external file included in every shader
    appendToShaderHeader(ShaderType::COUNT,
                        "#include \"nodeDataInput.cmn\"",
                         lineOffsets);

    Attorney::GLAPIShaderProgram::setGlobalLineOffset(
        lineOffsets[to_base(ShaderType::COUNT)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::VERTEX, lineOffsets[to_base(ShaderType::VERTEX)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::FRAGMENT, lineOffsets[to_base(ShaderType::FRAGMENT)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::GEOMETRY, lineOffsets[to_base(ShaderType::GEOMETRY)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::TESSELATION_CTRL,
        lineOffsets[to_base(ShaderType::TESSELATION_CTRL)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::TESSELATION_EVAL,
        lineOffsets[to_base(ShaderType::TESSELATION_EVAL)]);

    Attorney::GLAPIShaderProgram::addLineOffset(
        ShaderType::COMPUTE, lineOffsets[to_base(ShaderType::COMPUTE)]);

    // Check initialization status for GLSL and glsl-optimizer
    return glswState == 1;
}

/// Revert everything that was set up in "initShaders()"
bool GL_API::deInitShaders() {
    glShaderProgram::destroyStaticData();

    // Shutdown GLSW
    return (glswShutdown() == 1);
}

/// Try to find the requested font in the font cache. Load on cache miss.
I32 GL_API::getFont(const stringImpl& fontName) {
    if (_fontCache.first.compare(fontName) != 0) {
        _fontCache.first = fontName;
        U64 fontNameHash = _ID_RT(fontName);
        // Search for the requested font by name
        FontCache::const_iterator it = _fonts.find(fontNameHash);
        // If we failed to find it, it wasn't loaded yet
        if (it == std::cend(_fonts)) {
            // Fonts are stored in the general asset directory -> in the GUI
            // subfolder -> in the fonts subfolder
            stringImpl fontPath(Paths::g_assetsLocation + Paths::g_GUILocation + Paths::g_fontsPath);
            fontPath += fontName;
            // We use FontStash to load the font file
            _fontCache.second =
                fonsAddFont(_fonsContext, fontName.c_str(), fontPath.c_str());
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

    GL_API::setBlending(0,
                        true,
                        BlendingProperties {
                            BlendProperty::SRC_ALPHA,
                            BlendProperty::INV_SRC_ALPHA
                        });

    GL_API::setBlendColour(DefaultColours::DIVIDE_BLUE_U8);
    I32 width = _context.renderingResolution().w;
    I32 height = _context.renderingResolution().h;
        
    vectorAlg::vecSize drawCount = 0;

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
        
        const vectorImplFast<stringImpl>& text = entry.text();
        vectorAlg::vecSize lineCount = text.size();
        for (vectorAlg::vecSize i = 0; i < lineCount; ++i) {
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

void GL_API::drawIMGUI(ImDrawData* data) {
#if defined(USE_FIXED_FUNCTION_IMGUI)
    GL_API::setActiveVAO(s_imguiVAO);
#endif
    if (data != nullptr && data->Valid) {
        //glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

        GenericDrawCommand cmd(PrimitiveType::TRIANGLES, 0, 0);
        for (int n = 0; n < data->CmdListsCount; n++) {
            const ImDrawList* cmd_list = data->CmdLists[n];

#if defined(USE_FIXED_FUNCTION_IMGUI)
            const ImDrawIdx* idx_buffer_offset = 0;
            setActiveBuffer(GL_ARRAY_BUFFER, s_imguiVBO);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

            setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, s_imguiIB);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);
#else
            assert(cmd_list->VtxBuffer.size() < MAX_IMGUI_VERTS);
            _IMGUIBuffer->updateBuffer(0, to_U32(cmd_list->VtxBuffer.size()), 0, (bufferPtr)&cmd_list->VtxBuffer.front());
            _IMGUIBuffer->updateIndexBuffer(vectorImpl<U32>(std::cbegin(cmd_list->IdxBuffer), std::cend(cmd_list->IdxBuffer)));
#endif
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback) {
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    GL_API::bindTexture(0, (GLuint)((intptr_t)pcmd->TextureId));
                    GL_API::setScissor((I32)pcmd->ClipRect.x,
                                       (I32)(s_activeViewport.w - pcmd->ClipRect.w),
                                       (I32)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                                       (I32)(pcmd->ClipRect.w - pcmd->ClipRect.y));

                    cmd.cmd().indexCount = to_U32(pcmd->ElemCount);
#if defined(USE_FIXED_FUNCTION_IMGUI)
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
#else
                    _IMGUIBuffer->draw(cmd);
#endif
                }
#if defined(USE_FIXED_FUNCTION_IMGUI)
                idx_buffer_offset += pcmd->ElemCount;
#else
                cmd.cmd().firstIndex += pcmd->ElemCount;
#endif
            }
        }
        //glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    }
}

bool GL_API::switchWindow(I64 windowGUID) {
    if (windowGUID != -1 && windowGUID != s_activeWindowGUID) {
        DisplayWindow& window = _context.parent().platformContext().app().windowManager().getWindow(windowGUID);
        SDL_GL_MakeCurrent(window.getRawWindow(), GLUtil::_glRenderContext);
        s_activeWindowGUID = windowGUID;
        return true;
    }
    return false;
}

bool GL_API::bindPipeline(const Pipeline& pipeline) {
    if (s_activePipeline && *s_activePipeline == pipeline) {
        return true;
    }
    s_activePipeline = &pipeline;

    // Set the proper render states
    setStateBlock(pipeline.stateHash());

    glShaderProgram* program = static_cast<glShaderProgram*>(pipeline.shaderProgram());
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(program != nullptr, "GFXDevice error: Draw shader state is not valid for the current draw operation!");
    // Try to bind the shader program. If it failed to load, or isn't loaded yet, cancel the draw request for this frame
    if (Attorney::GLAPIShaderProgram::bind(*program)) {
        for (const ShaderFunctions::value_type& func : pipeline.shaderFunctions()) {
            Attorney::GLAPIShaderProgram::SetSubroutines(*program, func.first, func.second);
        }
        return true;
    }

    return false;
}

void GL_API::sendPushConstants(const PushConstants& pushConstants) {
    assert(s_activePipeline != nullptr);

    glShaderProgram* program = static_cast<glShaderProgram*>(s_activePipeline->shaderProgram());
    program->UploadPushConstants(pushConstants);
}

void GL_API::dispatchCompute(const ComputeParams& computeParams) {
    assert(s_activePipeline != nullptr);

    glDispatchCompute(computeParams._groupSize.x,
                      computeParams._groupSize.y,
                      computeParams._groupSize.z);

    //ToDo: Separate compute and memory barriers -Ionut
    if (computeParams._barrierType != MemoryBarrierType::COUNT) {
        MemoryBarrierMask barrierType = MemoryBarrierMask::GL_ALL_BARRIER_BITS;
        switch (computeParams._barrierType) {
            case MemoryBarrierType::ALL:
                break;
            case MemoryBarrierType::BUFFER:
                barrierType = MemoryBarrierMask::GL_BUFFER_UPDATE_BARRIER_BIT;
                break;
            case MemoryBarrierType::SHADER_BUFFER:
                barrierType = MemoryBarrierMask::GL_SHADER_STORAGE_BARRIER_BIT;
                break;
            case MemoryBarrierType::COUNTER:
                barrierType = MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT;
                break;
            case MemoryBarrierType::QUERY:
                barrierType = MemoryBarrierMask::GL_QUERY_BUFFER_BARRIER_BIT;
                break;
            case MemoryBarrierType::RENDER_TARGET:
                barrierType = MemoryBarrierMask::GL_FRAMEBUFFER_BARRIER_BIT;
                break;
            case MemoryBarrierType::TEXTURE:
                barrierType = MemoryBarrierMask::GL_TEXTURE_UPDATE_BARRIER_BIT;
                break;
            case MemoryBarrierType::TRANSFORM_FEEDBACK:
                barrierType = MemoryBarrierMask::GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
                break;
        }

        glMemoryBarrier(barrierType);
    }
}

bool GL_API::draw(const GenericDrawCommand& cmd) {
    if (cmd.sourceBuffer() == nullptr) {
        GL_API::setActiveVAO(s_dummyVAO);

        U32 indexCount = 0;
        switch (cmd.primitiveType()) {
            case PrimitiveType::TRIANGLES: indexCount = cmd.drawCount() * 3; break;
            case PrimitiveType::API_POINTS: indexCount = cmd.drawCount(); break;
            default: indexCount = cmd.cmd().indexCount; break;
        }

        glDrawArrays(GLUtil::glPrimitiveTypeTable[to_U32(cmd.primitiveType())], cmd.cmd().firstIndex, indexCount);
    } else {
        cmd.sourceBuffer()->draw(cmd);
    }

    return true;
}

void GL_API::pushDebugMessage(const char* message, I32 id) {
    if (Config::ENABLE_GPU_VALIDATION && config()._enableDebugMsgGroups) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, message);
    }
}

void GL_API::popDebugMessage() {
    if (Config::ENABLE_GPU_VALIDATION && config()._enableDebugMsgGroups) {
        glPopDebugGroup();
    }
}

void GL_API::flushCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    U32 drawCallCount = 0;

    const vectorImplFast<std::shared_ptr<GFX::Command>>& commands = commandBuffer();
    for (const std::shared_ptr<GFX::Command>& cmd : commands) {
        switch (cmd->_type) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                GFX::BeginRenderPassCommand* crtCmd = static_cast<GFX::BeginRenderPassCommand*>(cmd.get());
                glFramebuffer& rt = static_cast<glFramebuffer&>(_context.renderTargetPool().renderTarget(crtCmd->_target));
                Attorney::GLAPIRenderTarget::begin(rt, crtCmd->_descriptor);
                GL_API::s_activeRenderTarget = &rt;
            }break;
            case GFX::CommandType::END_RENDER_PASS: {
                assert(GL_API::s_activeRenderTarget != nullptr);
                Attorney::GLAPIRenderTarget::end(*GL_API::s_activeRenderTarget);
            }break;
            case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
                GFX::BeginPixelBufferCommand* crtCmd = static_cast<GFX::BeginPixelBufferCommand*>(cmd.get());
                assert(crtCmd->_buffer != nullptr);
                glPixelBuffer* buffer = static_cast<glPixelBuffer*>(crtCmd->_buffer);
                bufferPtr data = Attorney::GLAPIPixelBuffer::begin(*buffer);
                if (crtCmd->_command) {
                    crtCmd->_command(data);
                }
                GL_API::s_activePixelBuffer = buffer;
            }break;
            case GFX::CommandType::END_PIXEL_BUFFER: {
                assert(GL_API::s_activePixelBuffer != nullptr);
                Attorney::GLAPIPixelBuffer::end(*GL_API::s_activePixelBuffer);
            }break;
            case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
                assert(s_activeRenderTarget != nullptr);
                GFX::BeginRenderSubPassCommand* crtCmd = static_cast<GFX::BeginRenderSubPassCommand*>(cmd.get());
                GL_API::s_activeRenderTarget->setMipLevel(crtCmd->_mipWriteLevel);
            }break;
            case GFX::CommandType::END_RENDER_SUB_PASS: {
            }break;
            case GFX::CommandType::BLIT_RT: {
                GFX::BlitRenderTargetCommand* crtCmd = static_cast<GFX::BlitRenderTargetCommand*>(cmd.get());
                _context.renderTargetPool().renderTarget(crtCmd->_destination).blitFrom(&_context.renderTargetPool().renderTarget(crtCmd->_source),
                                                                                        crtCmd->_blitColour,
                                                                                        crtCmd->_blitDepth);
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                GFX::BindDescriptorSetsCommand* crtCmd = static_cast<GFX::BindDescriptorSetsCommand*>(cmd.get());
                const DescriptorSet& set = crtCmd->_set;

                makeTexturesResident(set._textureData);
                for (const ShaderBufferBinding& shaderBufCmd : set._shaderBuffers) {
                    shaderBufCmd._buffer->bindRange(shaderBufCmd._binding,
                                                    shaderBufCmd._range.x,
                                                    shaderBufCmd._range.y);
                    if (shaderBufCmd._atomicCounter.first) {
                        shaderBufCmd._buffer->bindAtomicCounter(shaderBufCmd._atomicCounter.second.x,
                                                                shaderBufCmd._atomicCounter.second.y);
                    }
                }
            }break;
            case GFX::CommandType::BIND_PIPELINE: {
                const Pipeline* pipeline = static_cast<GFX::BindPipelineCommand*>(cmd.get())->_pipeline;
                assert(pipeline != nullptr);
                bindPipeline(*pipeline);
            } break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                sendPushConstants(static_cast<GFX::SendPushConstantsCommand*>(cmd.get())->_constants);
            } break;
            case GFX::CommandType::SET_SCISSOR: {
                setScissor(static_cast<GFX::SetScissorCommand*>(cmd.get())->_rect);
            }break;
            case GFX::CommandType::SET_BLEND: {
                GFX::SetBlendCommand* blendCmd = static_cast<GFX::SetBlendCommand*>(cmd.get());
                setBlending(blendCmd->_enabled, blendCmd->_blendProperties);
            }break;
            case GFX::CommandType::SET_VIEWPORT: {
                _context.setViewport(static_cast<GFX::SetViewportCommand*>(cmd.get())->_viewport);
            }break;
            case GFX::CommandType::SET_CAMERA: {
                Attorney::GFXDeviceAPI::renderFromCamera(_context, *(static_cast<GFX::SetCameraCommand*>(cmd.get())->_camera));
            }break;
            case GFX::CommandType::SET_CLIP_PLANES: {
                Attorney::GFXDeviceAPI::setClippingPlanes(_context, static_cast<GFX::SetClipPlanesCommand*>(cmd.get())->_clippingPlanes);
            }break;
            case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
                 GFX::BeginDebugScopeCommand* crtCmd = static_cast<GFX::BeginDebugScopeCommand*>(cmd.get());
                 pushDebugMessage(crtCmd->_scopeName.c_str(), crtCmd->_scopeID);
            } break;
            case GFX::CommandType::END_DEBUG_SCOPE: {
                popDebugMessage();
            } break;
            case GFX::CommandType::DRAW_TEXT: {
                Attorney::GFXDeviceAPI::uploadGPUBlock(_context);

                GFX::DrawTextCommand* crtCmd = static_cast<GFX::DrawTextCommand*>(cmd.get());
                drawText(crtCmd->_batch);
            }break;
            case GFX::CommandType::SWITCH_WINDOW: {
                GFX::SwitchWindowCommand* crtCmd = static_cast<GFX::SwitchWindowCommand*>(cmd.get());
                switchWindow(crtCmd->windowGUID);
            }break;
            case GFX::CommandType::DRAW_IMGUI: {
                Attorney::GFXDeviceAPI::uploadGPUBlock(_context);

                GFX::DrawIMGUICommand* crtCmd = static_cast<GFX::DrawIMGUICommand*>(cmd.get());
                drawIMGUI(crtCmd->_data);
            }break;
            case GFX::CommandType::DRAW_COMMANDS : {
                Attorney::GFXDeviceAPI::uploadGPUBlock(_context);

                const vectorImpl<GenericDrawCommand>& drawCommands = static_cast<GFX::DrawCommand*>(cmd.get())->_drawCommands;
                for (const GenericDrawCommand& currentDrawCommand : drawCommands) {
                    if (draw(currentDrawCommand)) {
                        if (currentDrawCommand.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY)) {
                            drawCallCount++;
                        }
                        if (currentDrawCommand.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME)) {
                            drawCallCount++;
                        }
                    }
                }
            }break;
            case GFX::CommandType::DISPATCH_COMPUTE: {
                Attorney::GFXDeviceAPI::uploadGPUBlock(_context);

                GFX::DispatchComputeCommand* crtCmd = static_cast<GFX::DispatchComputeCommand*>(cmd.get());
                dispatchCompute(crtCmd->_params);
            }break;
        };
    }

    _context.registerDrawCalls(drawCallCount);
}

/// Activate the render state block described by the specified hash value (0 == default state block)
size_t GL_API::setStateBlock(size_t stateBlockHash) {
    // Passing 0 is a perfectly acceptable way of enabling the default render state block
    if (stateBlockHash == 0) {
        stateBlockHash = _context.getDefaultStateBlock(false);
    }

    return setStateBlockInternal(stateBlockHash);
}

void GL_API::registerCommandBuffer(const ShaderBuffer& commandBuffer) const {
    GLuint bufferID = static_cast<const glUniformBuffer&>(commandBuffer).bufferID();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, bufferID);
}

bool GL_API::makeTexturesResident(const TextureDataContainer& textureData) {
    STUBBED("ToDo: Optimise this: If over n textures, get max binding slot, create [0...maxSlot] bindings, fill unused with 0 and send as one command with glBindTextures -Ionut")
    if (false) {
        constexpr vectorAlg::vecSize k_textureThreshold = 3;
        if (textureData.textures().size() > k_textureThreshold) {
            GLushort offset = 0;
            vectorImpl<GLuint> handles;
            vectorImpl<GLuint> samplers;
            ///etc

            GL_API::bindTextures(offset, (GLuint)handles.size(), handles.data(), samplers.data());
        }
    }
    for (auto data : textureData.textures()) {
        makeTextureResident(data.first, data.second);
    }

    return true;
}

bool GL_API::makeTextureResident(const TextureData& textureData, U8 binding) {
    return bindTexture(
        static_cast<GLushort>(binding),
        textureData.getHandle(),
        textureData._samplerHash);
}

bool GL_API::changeViewportInternal(const vec4<I32>& viewport) {
    return changeViewport(viewport);
}

/// Verify if we have a sampler object created and available for the given
/// descriptor
size_t GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor) {
    // Get the descriptor's hash value
    size_t hashValue = descriptor.getHash();
    // Try to find the hash value in the sampler object map
    UpgradableReadLock ur_lock(s_samplerMapLock);
    // If we fail to find it, we need to create a new sampler object
    if (s_samplerMap.find(hashValue) == std::end(s_samplerMap)) {
        UpgradeToWriteLock w_lock(ur_lock);
        // Create and store the newly created sample object. GL_API is
        // responsible for deleting these!
        hashAlg::emplace(s_samplerMap, hashValue, descriptor);
    }
    // Return the sampler object's hash value
    return hashValue;
}

/// Return the OpenGL sampler object's handle for the given hash value
GLuint GL_API::getSamplerHandle(size_t samplerHash) {
    // If the hash value is 0, we assume the code is trying to unbind a sampler object
    if (samplerHash > 0) {
        // If we fail to find the sampler object for the given hash, we print an
        // error and return the default OpenGL handle
        ReadLock r_lock(s_samplerMapLock);
        samplerObjectMap::const_iterator it = s_samplerMap.find(samplerHash);
        if (it != std::cend(s_samplerMap)) {
            // Return the OpenGL handle for the sampler object matching the specified hash value
            return it->second.getObjectHandle();
        }
           
        Console::errorfn(Locale::get(_ID("ERROR_NO_SAMPLER_OBJECT_FOUND")), samplerHash);
    }

    return 0;
}

U32 GL_API::getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const {
    return to_U32(static_cast<const CEGUI::OpenGLTexture&>(textureIn).getOpenGLTexture());
}

};
