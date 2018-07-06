﻿#include "stdafx.h"

#include "Headers/GLWrapper.h"
#include "Headers/glIMPrimitive.h"
#include "Headers/glHardwareQuery.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
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

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#endif //CEGUI_STATIC

#include <CEGUI/CEGUI.h>

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
      _currentStateBlockHash(0),
      _previousStateBlockHash(0),
      _fonsContext(nullptr),
      _GUIGLrenderer(nullptr),
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT /* | GL_STENCIL_BUFFER_BIT*/);
    // Clears are registered as draw calls by most software, so we do the same
    // to stay in sync with third party software
    _context.registerDrawCall();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    _previousStateBlockHash = 0;
}

/// Finish rendering the current frame
void GL_API::endFrame() {
    // Revert back to the default OpenGL states
    clearStates();
    // Swap buffers
    const DisplayWindow& window = _context.parent().platformContext().app().windowManager().getActiveWindow();
    if (window.swapBuffers() && !window.minimized() && !window.hidden()) {
        Time::ScopedTimer time(_swapBufferTimer);
        SDL_GL_SwapWindow(window.getRawWindow());
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

void GL_API::pushDebugMessage(const char* message, I32 id) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, message);
}

void GL_API::popDebugMessage() {
    glPopDebugGroup();
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
    static vec4<U8> textBlendColour(Util::ToByteColour(DefaultColours::DIVIDE_BLUE()));

    static BlendingProperties textBlendProperties{
        BlendProperty::SRC_ALPHA,
        BlendProperty::INV_SRC_ALPHA
    };

    GFX::ScopedDebugMessage(_context, "OpenGL render text start!", 2);

    GL_API::setBlending(0, true, textBlendProperties, textBlendColour);
    
    I32 height = _context.getCurrentViewport().sizeY;
        
    vectorAlg::vecSize drawCount = 0;

    fonsClearState(_fonsContext);
    for (const TextElement& entry : batch())
    {
        const TextLabel& textLabel = *entry._textLabel;
        // Retrieve the font from the font cache
        I32 font = getFont(textLabel._font);
        // The font may be invalid, so skip this text label
        if (font != FONS_INVALID) {
            fonsSetFont(_fonsContext, font);
            fonsSetBlur(_fonsContext, textLabel._blurAmount);
            fonsSetBlur(_fonsContext, textLabel._spacing);
            fonsSetAlign(_fonsContext, textLabel._alignFlag);
            fonsSetSize(_fonsContext, to_F32(textLabel._fontSize));
            fonsSetColour(_fonsContext,
                            textLabel._colour.r,
                            textLabel._colour.g,
                            textLabel._colour.b,
                            textLabel._colour.a);

            F32 textX = entry._position.x;
            F32 textY = height - entry._position.y;
            F32 lh = 0;
            fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);

            const vectorImpl<stringImpl>& text = textLabel.text();
            vectorAlg::vecSize lineCount = text.size();
            for (vectorAlg::vecSize i = 0; i < lineCount; ++i) {
                fonsDrawText(_fonsContext,
                                textX,
                                textY - (lh * i),
                                text[i].c_str(),
                                nullptr);
            }
            drawCount += lineCount;
        }
        // Register each label rendered as a draw call
        _context.registerDrawCalls(to_U32(drawCount));
    }
}

bool GL_API::bindPipeline(const Pipeline& pipeline) {
    if (s_activePipeline && *s_activePipeline == pipeline) {
        return true;
    }
    s_activePipeline = &pipeline;

    // Set the proper render states
    setStateBlock(pipeline.stateHash());
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(pipeline.shaderProgram() != nullptr, "GFXDevice error: Draw shader state is not valid for the current draw operation!");
    // Try to bind the shader program. If it failed to load, or isn't loaded yet, cancel the draw request for this frame
    return pipeline.shaderProgram()->bind();
}

void GL_API::sendPushConstants(const PushConstants& pushConstants) {
    assert(s_activePipeline != nullptr);

    glShaderProgram* program = static_cast<glShaderProgram*>(s_activePipeline->shaderProgram());
    program->UploadPushConstants(pushConstants);
}

void GL_API::dispatchCompute(const ShaderProgram::ComputeParams& computeParams) {
    assert(s_activePipeline != nullptr);

    glShaderProgram* program = static_cast<glShaderProgram*>(s_activePipeline->shaderProgram());
    program->DispatchCompute(computeParams._groupSize.x,
                             computeParams._groupSize.y,
                             computeParams._groupSize.z);
    program->SetMemoryBarrier(computeParams._barrierType);
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

void GL_API::flushCommandBuffer(GFX::CommandBuffer& commandBuffer) {
    U32 drawCallCount = 0;
    for (const std::shared_ptr<GFX::Command>& cmd : commandBuffer()) {
        switch (cmd->_type) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                GFX::BeginRenderPassCommand* crtCmd = static_cast<GFX::BeginRenderPassCommand*>(cmd.get());
                _context.renderTargetPool().drawToTargetBegin(crtCmd->_target, crtCmd->_descriptor);
            }break;
            case GFX::CommandType::END_RENDER_PASS: {
                _context.renderTargetPool().drawToTargetEnd();
            }break;
            case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
                assert(s_activeRenderTarget != nullptr);
                GFX::BeginRenderSubPassCommand* crtCmd = static_cast<GFX::BeginRenderSubPassCommand*>(cmd.get());
                s_activeRenderTarget->setMipLevel(crtCmd->_mipWriteLevel);
            }break;
            case GFX::CommandType::END_RENDER_SUB_PASS: {
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
                bindPipeline(static_cast<GFX::BindPipelineCommand*>(cmd.get())->_pipeline);
            } break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                sendPushConstants(static_cast<GFX::SendPushConstantsCommand*>(cmd.get())->_constants);
            } break;
            case GFX::CommandType::SET_SCISSOR: {
                assert(false && "ToDo");
            }break;
            case GFX::CommandType::SET_VIEWPORT: {
                _context.setViewport(static_cast<GFX::SetViewportCommand*>(cmd.get())->_viewport);
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

    // If the new state hash is different from the previous one
    if (stateBlockHash != _currentStateBlockHash) {
        // Remember the previous state hash
        _previousStateBlockHash = _currentStateBlockHash;
        // Update the current state hash
        _currentStateBlockHash = stateBlockHash;

        bool currentStateValid = false;
        const RenderStateBlock& currentState = RenderStateBlock::get(_currentStateBlockHash, currentStateValid);
        if (_previousStateBlockHash != 0) {
            bool previousStateValid = false;
            const RenderStateBlock& previousState = RenderStateBlock::get(_previousStateBlockHash, previousStateValid);

            DIVIDE_ASSERT(currentStateValid && previousStateValid &&
                          currentState != previousState,
                          "GL_API error: Invalid state blocks detected on activation!");

            // Activate the new render state block in an rendering API dependent way
            activateStateBlock(currentState, previousState);
        } else {
            DIVIDE_ASSERT(currentStateValid, "GL_API error: Invalid state blocks detected on activation!");
            activateStateBlock(currentState);
        }
    }

    // Return the previous state hash
    return _previousStateBlockHash;
}

void GL_API::registerCommandBuffer(const ShaderBuffer& commandBuffer) const {
    GLuint bufferID = static_cast<const glUniformBuffer&>(commandBuffer).bufferID();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, bufferID);
}

bool GL_API::makeTexturesResident(const TextureDataContainer& textureData) {
    for (const TextureData& data : textureData.textures()) {
        makeTextureResident(data);
    }

    return true;
}

bool GL_API::makeTextureResident(const TextureData& textureData) {
    return bindTexture(
        static_cast<GLushort>(textureData.getHandleLow()),
        textureData.getHandleHigh(),
        textureData._samplerHash);
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

};
