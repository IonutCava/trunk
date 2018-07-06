#include "config.h"

#include "Headers/glResources.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include <GLIM/glim.h>

namespace Divide {

namespace {
    const U32 g_numWarmupVAOs = 25u;
};

glVAOPool::glVAOPool()
{
    
}

glVAOPool::~glVAOPool()
{
    destroy();
}

void glVAOPool::init(U32 capacity) {
    destroy();

    GLuint warmupVAOs[g_numWarmupVAOs] = {{0u}};
    glCreateVertexArrays(g_numWarmupVAOs, warmupVAOs);
    
    _pool.resize(capacity, std::make_pair(0, false));
    for (std::pair<GLuint, bool>& entry : _pool) {
        glCreateVertexArrays(1, &entry.first);
        if (Config::ENABLE_GPU_VALIDATION) {
            glObjectLabel(GL_VERTEX_ARRAY,
                          entry.first,
                          -1,
                          Util::StringFormat("DVD_VAO_%d", entry.first).c_str());
        }
    }


    glDeleteVertexArrays(g_numWarmupVAOs, warmupVAOs);
}

void glVAOPool::destroy() {
    for (std::pair<GLuint, bool>& entry : _pool) {
        glDeleteVertexArrays(1, &entry.first);
    }
    _pool.clear();
}

GLuint glVAOPool::allocate() {
    for (std::pair<GLuint, bool>& entry : _pool) {
        if (entry.second == false) {
            entry.second = true;
            return entry.first;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return 0;
}

void glVAOPool::allocate(U32 count, GLuint* vaosOUT) {
    for (U32 i = 0; i < count; ++i) {
        vaosOUT[i] = allocate();
    }
}

void glVAOPool::deallocate(GLuint& vao) {
    assert(Application::isMainThread());

    vectorImpl<std::pair<GLuint, bool>>::iterator it;
    it = std::find_if(std::begin(_pool),
                      std::end(_pool),
                      [vao](std::pair<GLuint, bool>& entry) {
                          return entry.first == vao;
                      });

    assert(it != std::cend(_pool));
    // We don't know what kind of state we may have in the current VAO so delete it and create a new one.
    glDeleteVertexArrays(1, &(it->first));
    glCreateVertexArrays(1, &(it->first));

    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_VERTEX_ARRAY,
                      it->first,
                      -1,
                      Util::StringFormat("DVD_VAO_%d", it->first).c_str());
    }
    it->second = false;
    vao = 0;
}


glHardwareQuery::glHardwareQuery() : _enabled(false),
    _queryID(0)
{
}

glHardwareQuery::~glHardwareQuery()
{
    destroy();
}

void glHardwareQuery::create() {
    destroy();
    glGenQueries(1, &_queryID);
}

void glHardwareQuery::destroy() {
    if (_queryID != 0) {
        glDeleteQueries(1, &_queryID);
    }
    _queryID = 0;
}


glHardwareQueryRing::glHardwareQueryRing(U32 queueLength)
    : RingBuffer(queueLength)
{
    _queries.resize(queueLength);
}

glHardwareQuery& glHardwareQueryRing::readQuery() {
    return _queries[queueReadIndex()];
}

glHardwareQuery& glHardwareQueryRing::writeQuery() {
    return _queries[queueWriteIndex()];
}

void glHardwareQueryRing::initQueries() {
    for (glHardwareQuery& query : _queries) {
       query.create();
       assert(query.getID() != 0 && "glHardwareQueryRing error: Invalid performance counter query ID!");
       // Initialize an initial time query as it solves certain issues with
       // consecutive queries later
       glBeginQuery(GL_TIME_ELAPSED, query.getID());
       glEndQuery(GL_TIME_ELAPSED);
       // Wait until the results are available
       GLint stopTimerAvailable = 0;
       while (!stopTimerAvailable) {
           glGetQueryObjectiv(query.getID(), GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
       }
    }
}

namespace GLUtil {

/*-----------Object Management----*/
GLuint _invalidObjectID = GL_INVALID_INDEX;
SDL_GLContext _glRenderContext;
SharedLock _glContextLock;
hashMapImpl<size_t /*threadID hash*/, SDL_GLContext> _glSecondaryContexts;
glVAOPool _vaoPool;

/// this may not seem very efficient (or useful) but it saves a lot of
/// single-use code scattered around further down
GLint getIntegerv(GLenum param) {
    GLint tempValue = 0;
    glGetIntegerv(param, &tempValue);
    return tempValue;
}

std::array<GLenum, to_const_uint(BlendProperty::COUNT)> glBlendTable;
std::array<GLenum, to_const_uint(BlendOperation::COUNT)> glBlendOpTable;
std::array<GLenum, to_const_uint(ComparisonFunction::COUNT)> glCompareFuncTable;
std::array<GLenum, to_const_uint(StencilOperation::COUNT)> glStencilOpTable;
std::array<GLenum, to_const_uint(CullMode::COUNT)> glCullModeTable;
std::array<GLenum, to_const_uint(FillMode::COUNT)> glFillModeTable;
std::array<GLenum, to_const_uint(TextureType::COUNT)> glTextureTypeTable;
std::array<GLenum, to_const_uint(GFXImageFormat::COUNT)> glImageFormatTable;
std::array<GLenum, to_const_uint(PrimitiveType::COUNT)> glPrimitiveTypeTable;
std::array<GLenum, to_const_uint(GFXDataFormat::COUNT)> glDataFormat;
std::array<GLenum, to_const_uint(TextureWrap::COUNT)> glWrapTable;
std::array<GLenum, to_const_uint(TextureFilter::COUNT)> glTextureFilterTable;
std::array<NS_GLIM::GLIM_ENUM, to_const_uint(PrimitiveType::COUNT)> glimPrimitiveType;
std::array<GLenum, to_const_uint(ShaderType::COUNT)> glShaderStageTable;
std::array<stringImpl, to_const_uint(ShaderType::COUNT)> glShaderStageNameTable;

void fillEnumTables() {
    glBlendTable[to_const_uint(BlendProperty::ZERO)] = GL_ZERO;
    glBlendTable[to_const_uint(BlendProperty::ONE)] = GL_ONE;
    glBlendTable[to_const_uint(BlendProperty::SRC_COLOR)] = GL_SRC_COLOR;
    glBlendTable[to_const_uint(BlendProperty::INV_SRC_COLOR)] = GL_ONE_MINUS_SRC_COLOR;
    glBlendTable[to_const_uint(BlendProperty::SRC_ALPHA)] = GL_SRC_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::INV_SRC_ALPHA)] = GL_ONE_MINUS_SRC_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::DEST_ALPHA)] = GL_DST_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::INV_DEST_ALPHA)] = GL_ONE_MINUS_DST_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::DEST_COLOR)] = GL_DST_COLOR;
    glBlendTable[to_const_uint(BlendProperty::INV_DEST_COLOR)] = GL_ONE_MINUS_DST_COLOR;
    glBlendTable[to_const_uint(BlendProperty::SRC_ALPHA_SAT)] = GL_SRC_ALPHA_SATURATE;

    glBlendOpTable[to_const_uint(BlendOperation::ADD)] = GL_FUNC_ADD;
    glBlendOpTable[to_const_uint(BlendOperation::SUBTRACT)] = GL_FUNC_SUBTRACT;
    glBlendOpTable[to_const_uint(BlendOperation::REV_SUBTRACT)] = GL_FUNC_REVERSE_SUBTRACT;
    glBlendOpTable[to_const_uint(BlendOperation::MIN)] = GL_MIN;
    glBlendOpTable[to_const_uint(BlendOperation::MAX)] = GL_MAX;

    glCompareFuncTable[to_const_uint(ComparisonFunction::NEVER)] = GL_NEVER;
    glCompareFuncTable[to_const_uint(ComparisonFunction::LESS)] = GL_LESS;
    glCompareFuncTable[to_const_uint(ComparisonFunction::EQUAL)] = GL_EQUAL;
    glCompareFuncTable[to_const_uint(ComparisonFunction::LEQUAL)] = GL_LEQUAL;
    glCompareFuncTable[to_const_uint(ComparisonFunction::GREATER)] = GL_GREATER;
    glCompareFuncTable[to_const_uint(ComparisonFunction::NEQUAL)] = GL_NOTEQUAL;
    glCompareFuncTable[to_const_uint(ComparisonFunction::GEQUAL)] = GL_GEQUAL;
    glCompareFuncTable[to_const_uint(ComparisonFunction::ALWAYS)] = GL_ALWAYS;

    glStencilOpTable[to_const_uint(StencilOperation::KEEP)] = GL_KEEP;
    glStencilOpTable[to_const_uint(StencilOperation::ZERO)] = GL_ZERO;
    glStencilOpTable[to_const_uint(StencilOperation::REPLACE)] = GL_REPLACE;
    glStencilOpTable[to_const_uint(StencilOperation::INCR)] = GL_INCR;
    glStencilOpTable[to_const_uint(StencilOperation::DECR)] = GL_DECR;
    glStencilOpTable[to_const_uint(StencilOperation::INV)] = GL_INVERT;
    glStencilOpTable[to_const_uint(StencilOperation::INCR_WRAP)] = GL_INCR_WRAP;
    glStencilOpTable[to_const_uint(StencilOperation::DECR_WRAP)] = GL_DECR_WRAP;

    glCullModeTable[to_const_uint(CullMode::CW)] = GL_BACK;
    glCullModeTable[to_const_uint(CullMode::CCW)] = GL_FRONT;
    glCullModeTable[to_const_uint(CullMode::ALL)] = GL_FRONT_AND_BACK;
    glCullModeTable[to_const_uint(CullMode::NONE)] = GL_NONE;

    glFillModeTable[to_const_uint(FillMode::POINT)] = GL_POINT;
    glFillModeTable[to_const_uint(FillMode::WIREFRAME)] = GL_LINE;
    glFillModeTable[to_const_uint(FillMode::SOLID)] = GL_FILL;

    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_1D)] = GL_TEXTURE_1D;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D)] = GL_TEXTURE_2D;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_3D)] = GL_TEXTURE_3D;
    // Modern OpenGL shouldn't distinguish between cube maps and cube map arrays.
    // A cube map is a cube map array of size 1
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_CUBE_MAP)] = GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_ARRAY)] = GL_TEXTURE_2D_ARRAY;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_CUBE_ARRAY)] = GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_MS)] = GL_TEXTURE_2D_MULTISAMPLE;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_ARRAY_MS)] = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE)] = GL_LUMINANCE;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA)] = GL_LUMINANCE_ALPHA;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA16F)] = gl::GL_LUMINANCE_ALPHA16F_ARB;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA32F)] = gl::GL_LUMINANCE_ALPHA32F_ARB;
    glImageFormatTable[to_const_uint(GFXImageFormat::INTENSITY)] = GL_INTENSITY;
    glImageFormatTable[to_const_uint(GFXImageFormat::ALPHA)] = GL_ALPHA;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED)] = GL_RED;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED8)] = GL_R8;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED16)] = GL_R16;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED16F)] = GL_R16F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED32)] = GL_R32UI;
    glImageFormatTable[to_const_uint(GFXImageFormat::RED32F)] = GL_R32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::BLUE)] = GL_BLUE;
    glImageFormatTable[to_const_uint(GFXImageFormat::GREEN)] = GL_GREEN;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG)] = GL_RG;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG8)] = GL_RG8;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG16)] = GL_RG16;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG16F)] = GL_RG16F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG32)] = GL_RG32UI;
    glImageFormatTable[to_const_uint(GFXImageFormat::RG32F)] = GL_RG32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB)] = GL_RGB;
    glImageFormatTable[to_const_uint(GFXImageFormat::BGR)] = GL_BGR;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB8)] = GL_RGB8;
    glImageFormatTable[to_const_uint(GFXImageFormat::SRGB8)] = GL_SRGB8;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB8I)] = GL_RGB8I;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB16)] = GL_RGB16;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB16F)] = GL_RGB16F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGB32F)] = GL_RGB32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::BGRA)] = GL_BGRA;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA)] = GL_RGBA;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA4)] = GL_RGBA4;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA8)] = GL_RGBA8;
    glImageFormatTable[to_const_uint(GFXImageFormat::SRGBA8)] = GL_SRGB8_ALPHA8;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA8I)] = GL_RGBA8I;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA16F)] = GL_RGBA16F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA32F)] = GL_RGBA32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT)] = GL_DEPTH_COMPONENT;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT16)] = GL_DEPTH_COMPONENT16;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT24)] = GL_DEPTH_COMPONENT24;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT32)] = GL_DEPTH_COMPONENT32;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT32F)] = GL_DEPTH_COMPONENT32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_RGB_DXT1)] = gl::GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_SRGB_DXT1)] = gl::GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_RGBA_DXT1)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT1)] = gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_RGBA_DXT3)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT3)] = gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_RGBA_DXT5)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    glImageFormatTable[to_const_uint(GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT5)] = gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::API_POINTS)] = GL_POINTS;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINES)] = GL_LINES;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINE_LOOP)] =  GL_LINE_LOOP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINE_STRIP)] = GL_LINE_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLES)] = GL_TRIANGLES;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLE_STRIP)] = GL_TRIANGLE_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLE_FAN)] = GL_TRIANGLE_FAN;
    // glPrimitiveTypeTable[to_const_uint(PrimitiveType::QUADS)] = GL_QUADS; //<Deprecated
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::QUAD_STRIP)] = GL_QUAD_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::POLYGON)] = GL_POLYGON;

    glDataFormat[to_const_uint(GFXDataFormat::UNSIGNED_BYTE)] = GL_UNSIGNED_BYTE;
    glDataFormat[to_const_uint(GFXDataFormat::UNSIGNED_SHORT)] = GL_UNSIGNED_SHORT;
    glDataFormat[to_const_uint(GFXDataFormat::UNSIGNED_INT)] = GL_UNSIGNED_INT;
    glDataFormat[to_const_uint(GFXDataFormat::SIGNED_BYTE)] = GL_BYTE;
    glDataFormat[to_const_uint(GFXDataFormat::SIGNED_SHORT)] = GL_SHORT;
    glDataFormat[to_const_uint(GFXDataFormat::SIGNED_INT)] = GL_INT;
    glDataFormat[to_const_uint(GFXDataFormat::FLOAT_16)] = GL_HALF_FLOAT;
    glDataFormat[to_const_uint(GFXDataFormat::FLOAT_32)] = GL_FLOAT;

    glWrapTable[to_const_uint(TextureWrap::MIRROR_REPEAT)] = GL_MIRRORED_REPEAT;
    glWrapTable[to_const_uint(TextureWrap::REPEAT)] = GL_REPEAT;
    glWrapTable[to_const_uint(TextureWrap::CLAMP)] = GL_CLAMP;
    glWrapTable[to_const_uint(TextureWrap::CLAMP_TO_EDGE)] = GL_CLAMP_TO_EDGE;
    glWrapTable[to_const_uint(TextureWrap::CLAMP_TO_BORDER)] = GL_CLAMP_TO_BORDER;
    glWrapTable[to_const_uint(TextureWrap::DECAL)] = GL_DECAL;

    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR)] = GL_LINEAR;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST)] = GL_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST_MIPMAP_NEAREST)] = GL_NEAREST_MIPMAP_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR_MIPMAP_NEAREST)] = GL_LINEAR_MIPMAP_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST_MIPMAP_LINEAR)] = GL_NEAREST_MIPMAP_LINEAR;
    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR_MIPMAP_LINEAR)] = GL_LINEAR_MIPMAP_LINEAR;

    glimPrimitiveType[to_const_uint(PrimitiveType::API_POINTS)] = NS_GLIM::GLIM_ENUM::GLIM_POINTS;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINES)] = NS_GLIM::GLIM_ENUM::GLIM_LINES;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINE_LOOP)] = NS_GLIM::GLIM_ENUM::GLIM_LINE_LOOP;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINE_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_LINE_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLES)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLES;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLE_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLE_FAN)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_FAN;
    glimPrimitiveType[to_const_uint(PrimitiveType::QUAD_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_QUAD_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::POLYGON)] = NS_GLIM::GLIM_ENUM::GLIM_POLYGON;

    glShaderStageTable[to_const_uint(ShaderType::VERTEX)] = GL_VERTEX_SHADER;
    glShaderStageTable[to_const_uint(ShaderType::FRAGMENT)] = GL_FRAGMENT_SHADER;
    glShaderStageTable[to_const_uint(ShaderType::GEOMETRY)] = GL_GEOMETRY_SHADER;
    glShaderStageTable[to_const_uint(ShaderType::TESSELATION_CTRL)] = GL_TESS_CONTROL_SHADER;
    glShaderStageTable[to_const_uint(ShaderType::TESSELATION_EVAL)] = GL_TESS_EVALUATION_SHADER;
    glShaderStageTable[to_const_uint(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER;

    glShaderStageNameTable[to_const_uint(ShaderType::VERTEX)] = "Vertex";
    glShaderStageNameTable[to_const_uint(ShaderType::FRAGMENT)] = "Fragment";
    glShaderStageNameTable[to_const_uint(ShaderType::GEOMETRY)] = "Geometry";
    glShaderStageNameTable[to_const_uint(ShaderType::TESSELATION_CTRL)] = "TessellationC";
    glShaderStageNameTable[to_const_uint(ShaderType::TESSELATION_EVAL)] = "TessellationE";
    glShaderStageNameTable[to_const_uint(ShaderType::COMPUTE)] = "Compute";
}

namespace {
void submitMultiIndirectCommand(U32 cmdOffset,
                                U32 drawCount,
                                GLenum mode,
                                GLenum internalFormat,
                                GLuint indexBuffer) {
    static const size_t cmdSize = sizeof(IndirectDrawCommand);
    if (indexBuffer > 0) {
        glMultiDrawElementsIndirect(mode, internalFormat, (bufferPtr)(cmdOffset * cmdSize), drawCount, cmdSize);
    } else {
        glMultiDrawArraysIndirect(mode, (bufferPtr)(cmdOffset * cmdSize), drawCount, cmdSize);
    }
}

void submitIndirectCommand(U32 cmdOffset,
                           GLenum mode,
                           GLenum internalFormat,
                           GLuint indexBuffer) {

    static const size_t cmdSize = sizeof(IndirectDrawCommand);
    if (indexBuffer > 0) {
        glDrawElementsIndirect(mode, internalFormat, (bufferPtr)(cmdOffset * cmdSize));
    } else {
        glDrawArraysIndirect(mode, (bufferPtr)(cmdOffset * cmdSize));
    }
}

void sumitDirectCommand(const IndirectDrawCommand& cmd,
                        GLenum mode,
                        GLenum internalFormat,
                        GLuint indexBuffer) {

    if (indexBuffer > 0) {
        glDrawElements(mode, cmd.indexCount, internalFormat, bufferOffset(cmd.firstIndex));
    } else {
        glDrawArrays(mode, cmd.firstIndex, cmd.indexCount);
    }
}

void submitDirectMultiCommand(const IndirectDrawCommand& cmd,
                              U32 drawCount,
                              GLenum mode,
                              GLenum internalFormat,
                              GLuint indexBuffer) {
    vectorImpl<GLsizei> count(drawCount, cmd.indexCount);
    if (indexBuffer > 0) {
        vectorImpl<U32> indices(drawCount, cmd.baseInstance);
        glMultiDrawElements(mode, count.data(), internalFormat, (bufferPtr*)(indices.data()), drawCount);
    } else {
        vectorImpl<I32> first(drawCount, cmd.firstIndex);
        glMultiDrawArrays(mode, first.data(), count.data(), drawCount);
    }
}

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         GLenum mode,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLuint indexBuffer) {

    if (useIndirectBuffer) {
        // Don't trust the driver to optimize the loop. Do it here so we know the cost upfront
        if (drawCommand.drawCount() > 1) {
            submitMultiIndirectCommand(drawCommand.commandOffset(), drawCommand.drawCount(), mode, internalFormat, indexBuffer);
        } else if (drawCommand.drawCount() == 1) {
            submitIndirectCommand(drawCommand.commandOffset(), mode, internalFormat, indexBuffer);
        }
    } else {
        if (drawCommand.drawCount() > 1) {
            submitDirectMultiCommand(drawCommand.cmd(), drawCommand.drawCount(), mode, internalFormat, indexBuffer);
        } else if (drawCommand.drawCount() == 1) {
            sumitDirectCommand(drawCommand.cmd(), mode, internalFormat, indexBuffer);
        }
    }
}

};

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLuint indexBuffer) {
    // Process the actual draw command
    if (Config::Profile::DISABLE_DRAWS) {
        return;
    }

    DIVIDE_ASSERT(drawCommand.primitiveType() != PrimitiveType::COUNT,
                  "GLUtil::submitRenderCommand error: Draw command's type is not valid!");
    
    GL_API::toggleRasterization(!drawCommand.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_NO_RASTERIZE));
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    if (drawCommand.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_GEOMETRY)) {
        GLenum mode = glPrimitiveTypeTable[to_uint(drawCommand.primitiveType())];
        submitRenderCommand(drawCommand, mode, useIndirectBuffer, internalFormat, indexBuffer);
    }
    if (drawCommand.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_WIREFRAME)) {
        submitRenderCommand(drawCommand, GL_LINE_LOOP, useIndirectBuffer, internalFormat, indexBuffer);
    }
}

};  // namespace GLutil

}; //namespace Divide