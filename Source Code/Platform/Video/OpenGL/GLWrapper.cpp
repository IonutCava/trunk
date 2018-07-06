#include "stdafx.h"

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
    const U32 g_performanceQueryRingLength = 2;
    /// Number of queries
    const U32 g_performanceQueryCount = 1;
    /// ID of the frame duration query
    const U32 g_performanceQueryFrameDurationIndex = 0;
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
    FRAME_DURATION_GPU = 0;
    // All clip planes are disabled at first (default OpenGL state)
    _activeClipPlanes.fill(false);
    _fontCache.second = -1;
    s_samplerBoundMap.fill(0);
    s_textureBoundMap.fill(std::make_pair(0, GL_NONE));

    _hardwareQueries.reserve(g_performanceQueryCount);
    for (U32 i = 0; i < g_performanceQueryCount; ++i) {
        _hardwareQueries.emplace_back(std::make_shared<glHardwareQueryRing>(context, g_performanceQueryRingLength));
    }
}

GL_API::~GL_API()
{
    _hardwareQueries.clear();
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
        glBeginQuery(GL_TIME_ELAPSED, _hardwareQueries[g_performanceQueryFrameDurationIndex]->writeQuery().getID());
    }
    // Clear our buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT /* | GL_STENCIL_BUFFER_BIT*/);
    // Clears are registered as draw calls by most software, so we do the same
    // to stay in sync with third party software
    _context.registerDrawCall();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectDrawBuffer);
    _previousStateBlockHash = 0;
}

/// Finish rendering the current frame
void GL_API::endFrame(bool swapBuffers) {
    // Revert back to the default OpenGL states
    clearStates();
    // Swap buffers
    if (swapBuffers) {
        Time::ScopedTimer time(_swapBufferTimer);
        SDL_GL_SwapWindow(_context.parent().platformContext().app().windowManager().getActiveWindow().getRawWindow());
    }

    // End the timing query started in beginFrame() in debug builds
    if (Config::ENABLE_GPU_VALIDATION) {
        glEndQuery(GL_TIME_ELAPSED);
        _hardwareQueries[g_performanceQueryFrameDurationIndex]->incQueue();
    }
}

GLuint64 GL_API::getFrameDurationGPU() {
    if (Config::ENABLE_GPU_VALIDATION) {
        // The returned results are 'g_performanceQueryFrameDurationIndex' frames old!
        glGetQueryObjectui64v(_hardwareQueries[g_performanceQueryFrameDurationIndex]->readQuery().getID(),
                              GL_QUERY_RESULT,
                              &FRAME_DURATION_GPU);
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
                         Util::StringFormat("#define GPU_VENDOR %d", to_U32(_context.getGPUVendor())),
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
    if (_context.getGPUVendor() == GPUVendor::NVIDIA) {
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
        "#define SHADOW_SINGLE_MAP_ARRAY " +
            to_stringImpl(to_U32(ShaderProgram::TextureUsage::SHADOW_SINGLE)),
        lineOffsets);

    appendToShaderHeader(
        ShaderType::FRAGMENT,
        "#define SHADOW_LAYERED_MAP_ARRAY " +
            to_stringImpl( to_U32(ShaderProgram::TextureUsage::SHADOW_LAYERED)),
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
            hashAlg::emplace(_fonts, fontNameHash, _fontCache.second);
            
        } else {
            _fontCache.second = it->second;
        }

    }

    // Return the font
    return _fontCache.second;
}

/// Text rendering is handled exclusively by Mikko Mononen's FontStash library
/// (https://github.com/memononen/fontstash)
/// with his OpenGL frontend adapted for core context profiles
void GL_API::drawText(const vectorImpl<GUITextBatchEntry>& batch) {
    if (Config::ENABLE_GPU_VALIDATION) {
        pushDebugMessage("OpenGL render text start!", 2);
    }

    const RenderTarget& screenRT = _context.renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    U16 height = screenRT.getHeight();
        
    fonsClearState(_fonsContext);
    for (const GUITextBatchEntry& entry : batch)
    {
        const TextLabel& textLabel = *entry._textLabel;
        // Retrieve the font from the font cache
        I32 font = getFont(textLabel._font);
        // The font may be invalid, so skip this text label
        if (font != FONS_INVALID) {
            setStateBlock(entry._stateHash);

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

            F32 lh = 0;
            fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);

            const vectorImpl<stringImpl>& text = textLabel.text();
            vec2<F32> position(entry._position.x, height - entry._position.y);

            vectorAlg::vecSize lineCount = text.size();
            for (vectorAlg::vecSize i = 0; i < lineCount; ++i) {
                fonsDrawText(_fonsContext,
                             position.x,
                             position.y - (lh * i),
                             text[i].c_str(),
                             nullptr);
                // Register each label rendered as a draw call
                _context.registerDrawCall();
            }
        }
    }

    if (Config::ENABLE_GPU_VALIDATION) {
        popDebugMessage();
    }
}

bool GL_API::setState(const GenericDrawCommand& cmd) {
    // Set the proper render states
    setStateBlock(cmd.pipeline().stateHash());
    // We need a valid shader as no fixed function pipeline is available
    DIVIDE_ASSERT(cmd.pipeline().shaderProgram() != nullptr,
                  "GFXDevice error: Draw shader state is not valid for the current draw operation!");
    // Try to bind the shader program. If it failed to load, or isn't loaded
    // yet, cancel the draw request for this frame
    return cmd.pipeline().shaderProgram()->bind();
}

bool GL_API::draw(const GenericDrawCommand& cmd) {
    if (setState(cmd)) {
        if (cmd.sourceBuffer() == nullptr) {
            GL_API::setActiveVAO(s_dummyVAO);
        
            U32 indexCount = 0;
            switch(cmd.primitiveType()) {
                case PrimitiveType::TRIANGLES : indexCount = cmd.drawCount() * 3; break;
                case PrimitiveType::API_POINTS: indexCount = cmd.drawCount(); break;
                default: indexCount = cmd.cmd().indexCount; break;
            }

            glDrawArrays(GLUtil::glPrimitiveTypeTable[to_U32(cmd.primitiveType())], cmd.cmd().firstIndex, indexCount);
        } else {
            cmd.sourceBuffer()->draw(cmd);
        }

        return true;
    }

    return false;
}

void GL_API::flushCommandBuffer(const CommandBuffer& commandBuffer) {
    U32 drawCallCount = 0;
    for (const RenderPassCmd& pass : commandBuffer) {
        RenderTarget* target = nullptr; 
        if (pass._renderTarget._usage != RenderTargetUsage::COUNT) {
            target = &_context.renderTarget(pass._renderTarget);
            target->begin(pass._renderTargetDescriptor);
        }

        for (const RenderSubPassCmd& subPass : pass._subPassCmds) {
            makeTexturesResident(subPass._textures);
            for (const ShaderBufferBindCmd& shaderBufCmd : subPass._shaderBuffers) {
                shaderBufCmd._buffer->bindRange(shaderBufCmd._binding,
                                                shaderBufCmd._dataRange.x,
                                                shaderBufCmd._dataRange.y);
            }
            Attorney::GFXDeviceAPI::onRenderSubPass(_context);
            for (const GenericDrawCommand& cmd : subPass._commands) {
                if (draw(cmd)) {
                    if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY)) {
                        drawCallCount++;
                    }
                    if (cmd.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME)) {
                        drawCallCount++;
                    }
                }
            }
        }

        if (target != nullptr) {
            target->end();
        }
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
    s_indirectDrawBuffer = static_cast<const glUniformBuffer&>(commandBuffer).bufferID();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectDrawBuffer);
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
        GLUtil::glTextureTypeTable[to_U32(textureData._textureType)],
        textureData._samplerHash);
}

/// Called right before a glDraw* command
void GL_API::preCommandSubmission() {
    ShaderProgram::preCommandSubmission();
}

/// Called right after a glDraw* command returns
void GL_API::postCommandSubmission() {
    ShaderProgram::postCommandSubmission();
}

/// Verify if we have a sampler object created and available for the given
/// descriptor
size_t GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor) {
    // Get the descriptor's hash value
    size_t hashValue = descriptor.getHash();
    // Try to find the hash value in the sampler object map
    UpgradableReadLock ur_lock(s_samplerMapLock);
    samplerObjectMap::const_iterator it = s_samplerMap.find(hashValue);
    // If we fail to find it, we need to create a new sampler object
    if (it == std::end(s_samplerMap)) {
        UpgradeToWriteLock w_lock(ur_lock);
        // Create and store the newly created sample object. GL_API is
        // responsible for deleting these!
        hashAlg::emplace(s_samplerMap, hashValue, MemoryManager_NEW glSamplerObject(descriptor));
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
            return it->second->getObjectHandle();
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_NO_SAMPLER_OBJECT_FOUND")), samplerHash);
        }
    }

    return 0;
}

};
