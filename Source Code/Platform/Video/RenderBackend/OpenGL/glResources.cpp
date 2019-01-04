#include "stdafx.h"

#include "config.h"

#include "Headers/glResources.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"

#include <GLIM/glim.h>

namespace Divide {

glObject::glObject(glObjectType type, GFXDevice& context)
    : _type(type)
{
}

VAOBindings::VAOBindings() noexcept
    : _maxBindings(0)
{
}

VAOBindings::~VAOBindings()
{
}

void VAOBindings::init(U32 maxBindings) {
    _maxBindings = maxBindings;
}

const VAOBindings::BufferBindingParams& VAOBindings::bindingParams(GLuint vao, GLuint index) {
    VAOBufferData& data = _bindings[vao];
    vec_size count = data.size();
    if (count > 0) {
        assert(index <= count);
        return data[index];
    }

    assert(_maxBindings != 0);
    data.resize(_maxBindings);
    return data.front();
}

void VAOBindings::bindingParams(GLuint vao, GLuint index, const BufferBindingParams& newParams) {
    VAOBufferData& data = _bindings[vao];
    vec_size count = data.size();
    assert(count > 0 && count > index);
    ACKNOWLEDGE_UNUSED(count);

    data[index] = newParams;
}

namespace GLUtil {

/*-----------Object Management----*/
GLuint _invalidObjectID = GL_INVALID_INDEX;
GLuint _lastQueryResult = GL_INVALID_INDEX;

const DisplayWindow* _glMainRenderWindow;
thread_local SDL_GLContext _glSecondaryContext = nullptr;
std::mutex _glSecondaryContextMutex;

/// this may not seem very efficient (or useful) but it saves a lot of
/// single-use code scattered around further down
GLint getIntegerv(GLenum param) {
    GLint tempValue = 0;
    glGetIntegerv(param, &tempValue);
    return tempValue;
}

std::array<GLenum, to_base(BlendProperty::COUNT)> glBlendTable;
std::array<GLenum, to_base(BlendOperation::COUNT)> glBlendOpTable;
std::array<GLenum, to_base(ComparisonFunction::COUNT)> glCompareFuncTable;
std::array<GLenum, to_base(StencilOperation::COUNT)> glStencilOpTable;
std::array<GLenum, to_base(CullMode::COUNT)> glCullModeTable;
std::array<GLenum, to_base(FillMode::COUNT)> glFillModeTable;
std::array<GLenum, to_base(TextureType::COUNT)> glTextureTypeTable;
std::array<GLenum, to_base(GFXImageFormat::COUNT)> glImageFormatTable;
std::array<GLenum, to_base(PrimitiveType::COUNT)> glPrimitiveTypeTable;
std::array<GLenum, to_base(GFXDataFormat::COUNT)> glDataFormat;
std::array<GLenum, to_base(TextureWrap::COUNT)> glWrapTable;
std::array<GLenum, to_base(TextureFilter::COUNT)> glTextureFilterTable;
std::array<NS_GLIM::GLIM_ENUM, to_base(PrimitiveType::COUNT)> glimPrimitiveType;
std::array<GLenum, to_base(ShaderType::COUNT)> glShaderStageTable;
std::array<UseProgramStageMask, to_base(ShaderType::COUNT) + 1> glProgramStageMask;
std::array<stringImpl, to_base(ShaderType::COUNT)> glShaderStageNameTable;
std::array<GLenum, to_base(QueryType::COUNT)> glQueryTypeTable;

void fillEnumTables() {
    glBlendTable[to_base(BlendProperty::ZERO)] = GL_ZERO;
    glBlendTable[to_base(BlendProperty::ONE)] = GL_ONE;
    glBlendTable[to_base(BlendProperty::SRC_COLOR)] = GL_SRC_COLOR;
    glBlendTable[to_base(BlendProperty::INV_SRC_COLOR)] = GL_ONE_MINUS_SRC_COLOR;
    glBlendTable[to_base(BlendProperty::SRC_ALPHA)] = GL_SRC_ALPHA;
    glBlendTable[to_base(BlendProperty::INV_SRC_ALPHA)] = GL_ONE_MINUS_SRC_ALPHA;
    glBlendTable[to_base(BlendProperty::DEST_ALPHA)] = GL_DST_ALPHA;
    glBlendTable[to_base(BlendProperty::INV_DEST_ALPHA)] = GL_ONE_MINUS_DST_ALPHA;
    glBlendTable[to_base(BlendProperty::DEST_COLOR)] = GL_DST_COLOR;
    glBlendTable[to_base(BlendProperty::INV_DEST_COLOR)] = GL_ONE_MINUS_DST_COLOR;
    glBlendTable[to_base(BlendProperty::SRC_ALPHA_SAT)] = GL_SRC_ALPHA_SATURATE;

    glBlendOpTable[to_base(BlendOperation::ADD)] = GL_FUNC_ADD;
    glBlendOpTable[to_base(BlendOperation::SUBTRACT)] = GL_FUNC_SUBTRACT;
    glBlendOpTable[to_base(BlendOperation::REV_SUBTRACT)] = GL_FUNC_REVERSE_SUBTRACT;
    glBlendOpTable[to_base(BlendOperation::MIN)] = GL_MIN;
    glBlendOpTable[to_base(BlendOperation::MAX)] = GL_MAX;

    glCompareFuncTable[to_base(ComparisonFunction::NEVER)] = GL_NEVER;
    glCompareFuncTable[to_base(ComparisonFunction::LESS)] = GL_LESS;
    glCompareFuncTable[to_base(ComparisonFunction::EQUAL)] = GL_EQUAL;
    glCompareFuncTable[to_base(ComparisonFunction::LEQUAL)] = GL_LEQUAL;
    glCompareFuncTable[to_base(ComparisonFunction::GREATER)] = GL_GREATER;
    glCompareFuncTable[to_base(ComparisonFunction::NEQUAL)] = GL_NOTEQUAL;
    glCompareFuncTable[to_base(ComparisonFunction::GEQUAL)] = GL_GEQUAL;
    glCompareFuncTable[to_base(ComparisonFunction::ALWAYS)] = GL_ALWAYS;

    glStencilOpTable[to_base(StencilOperation::KEEP)] = GL_KEEP;
    glStencilOpTable[to_base(StencilOperation::ZERO)] = GL_ZERO;
    glStencilOpTable[to_base(StencilOperation::REPLACE)] = GL_REPLACE;
    glStencilOpTable[to_base(StencilOperation::INCR)] = GL_INCR;
    glStencilOpTable[to_base(StencilOperation::DECR)] = GL_DECR;
    glStencilOpTable[to_base(StencilOperation::INV)] = GL_INVERT;
    glStencilOpTable[to_base(StencilOperation::INCR_WRAP)] = GL_INCR_WRAP;
    glStencilOpTable[to_base(StencilOperation::DECR_WRAP)] = GL_DECR_WRAP;

    glCullModeTable[to_base(CullMode::CW)] = GL_BACK;
    glCullModeTable[to_base(CullMode::CCW)] = GL_FRONT;
    glCullModeTable[to_base(CullMode::ALL)] = GL_FRONT_AND_BACK;
    glCullModeTable[to_base(CullMode::NONE)] = GL_NONE;

    glFillModeTable[to_base(FillMode::POINT)] = GL_POINT;
    glFillModeTable[to_base(FillMode::WIREFRAME)] = GL_LINE;
    glFillModeTable[to_base(FillMode::SOLID)] = GL_FILL;

    glTextureTypeTable[to_base(TextureType::TEXTURE_1D)] = GL_TEXTURE_1D;
    glTextureTypeTable[to_base(TextureType::TEXTURE_2D)] = GL_TEXTURE_2D;
    glTextureTypeTable[to_base(TextureType::TEXTURE_3D)] = GL_TEXTURE_3D;
    // Modern OpenGL shouldn't distinguish between cube maps and cube map arrays.
    // A cube map is a cube map array of size 1
    glTextureTypeTable[to_base(TextureType::TEXTURE_CUBE_MAP)] = GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[to_base(TextureType::TEXTURE_2D_ARRAY)] = GL_TEXTURE_2D_ARRAY;
    glTextureTypeTable[to_base(TextureType::TEXTURE_CUBE_ARRAY)] = GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[to_base(TextureType::TEXTURE_2D_MS)] = GL_TEXTURE_2D_MULTISAMPLE;
    glTextureTypeTable[to_base(TextureType::TEXTURE_2D_ARRAY_MS)] = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    glImageFormatTable[to_base(GFXImageFormat::RED)] = GL_RED;
    glImageFormatTable[to_base(GFXImageFormat::RG)] = GL_RG;
    glImageFormatTable[to_base(GFXImageFormat::RGB)] = GL_RGB;
    glImageFormatTable[to_base(GFXImageFormat::BGR)] = GL_BGR;
    glImageFormatTable[to_base(GFXImageFormat::BGRA)] = GL_BGRA;
    glImageFormatTable[to_base(GFXImageFormat::RGBA)] = GL_RGBA;
    glImageFormatTable[to_base(GFXImageFormat::DEPTH_COMPONENT)] = GL_DEPTH_COMPONENT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGB_DXT1)] = gl::GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT1)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT3)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT5)] = gl::GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    
    glPrimitiveTypeTable[to_base(PrimitiveType::API_POINTS)] = GL_POINTS;
    glPrimitiveTypeTable[to_base(PrimitiveType::LINES)] = GL_LINES;
    glPrimitiveTypeTable[to_base(PrimitiveType::LINE_LOOP)] =  GL_LINE_LOOP;
    glPrimitiveTypeTable[to_base(PrimitiveType::LINE_STRIP)] = GL_LINE_STRIP;
    glPrimitiveTypeTable[to_base(PrimitiveType::TRIANGLES)] = GL_TRIANGLES;
    glPrimitiveTypeTable[to_base(PrimitiveType::TRIANGLE_STRIP)] = GL_TRIANGLE_STRIP;
    glPrimitiveTypeTable[to_base(PrimitiveType::TRIANGLE_FAN)] = GL_TRIANGLE_FAN;
    // glPrimitiveTypeTable[to_base(PrimitiveType::QUADS)] = GL_QUADS; //<Deprecated
    glPrimitiveTypeTable[to_base(PrimitiveType::QUAD_STRIP)] = GL_QUAD_STRIP;
    glPrimitiveTypeTable[to_base(PrimitiveType::POLYGON)] = GL_POLYGON;
    glPrimitiveTypeTable[to_base(PrimitiveType::PATCH)] = GL_PATCHES;

    glDataFormat[to_base(GFXDataFormat::UNSIGNED_BYTE)] = GL_UNSIGNED_BYTE;
    glDataFormat[to_base(GFXDataFormat::UNSIGNED_SHORT)] = GL_UNSIGNED_SHORT;
    glDataFormat[to_base(GFXDataFormat::UNSIGNED_INT)] = GL_UNSIGNED_INT;
    glDataFormat[to_base(GFXDataFormat::SIGNED_BYTE)] = GL_BYTE;
    glDataFormat[to_base(GFXDataFormat::SIGNED_SHORT)] = GL_SHORT;
    glDataFormat[to_base(GFXDataFormat::SIGNED_INT)] = GL_INT;
    glDataFormat[to_base(GFXDataFormat::FLOAT_16)] = GL_HALF_FLOAT;
    glDataFormat[to_base(GFXDataFormat::FLOAT_32)] = GL_FLOAT;

    glWrapTable[to_base(TextureWrap::MIRROR_REPEAT)] = GL_MIRRORED_REPEAT;
    glWrapTable[to_base(TextureWrap::REPEAT)] = GL_REPEAT;
    glWrapTable[to_base(TextureWrap::CLAMP)] = GL_CLAMP;
    glWrapTable[to_base(TextureWrap::CLAMP_TO_EDGE)] = GL_CLAMP_TO_EDGE;
    glWrapTable[to_base(TextureWrap::CLAMP_TO_BORDER)] = GL_CLAMP_TO_BORDER;
    glWrapTable[to_base(TextureWrap::DECAL)] = GL_DECAL;

    glTextureFilterTable[to_base(TextureFilter::LINEAR)] = GL_LINEAR;
    glTextureFilterTable[to_base(TextureFilter::NEAREST)] = GL_NEAREST;
    glTextureFilterTable[to_base(TextureFilter::NEAREST_MIPMAP_NEAREST)] = GL_NEAREST_MIPMAP_NEAREST;
    glTextureFilterTable[to_base(TextureFilter::LINEAR_MIPMAP_NEAREST)] = GL_LINEAR_MIPMAP_NEAREST;
    glTextureFilterTable[to_base(TextureFilter::NEAREST_MIPMAP_LINEAR)] = GL_NEAREST_MIPMAP_LINEAR;
    glTextureFilterTable[to_base(TextureFilter::LINEAR_MIPMAP_LINEAR)] = GL_LINEAR_MIPMAP_LINEAR;

    glimPrimitiveType[to_base(PrimitiveType::API_POINTS)] = NS_GLIM::GLIM_ENUM::GLIM_POINTS;
    glimPrimitiveType[to_base(PrimitiveType::LINES)] = NS_GLIM::GLIM_ENUM::GLIM_LINES;
    glimPrimitiveType[to_base(PrimitiveType::LINE_LOOP)] = NS_GLIM::GLIM_ENUM::GLIM_LINE_LOOP;
    glimPrimitiveType[to_base(PrimitiveType::LINE_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_LINE_STRIP;
    glimPrimitiveType[to_base(PrimitiveType::TRIANGLES)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLES;
    glimPrimitiveType[to_base(PrimitiveType::TRIANGLE_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_STRIP;
    glimPrimitiveType[to_base(PrimitiveType::TRIANGLE_FAN)] = NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_FAN;
    glimPrimitiveType[to_base(PrimitiveType::QUAD_STRIP)] = NS_GLIM::GLIM_ENUM::GLIM_QUAD_STRIP;
    glimPrimitiveType[to_base(PrimitiveType::POLYGON)] = NS_GLIM::GLIM_ENUM::GLIM_POLYGON;

    glShaderStageTable[to_base(ShaderType::VERTEX)] = GL_VERTEX_SHADER;
    glShaderStageTable[to_base(ShaderType::FRAGMENT)] = GL_FRAGMENT_SHADER;
    glShaderStageTable[to_base(ShaderType::GEOMETRY)] = GL_GEOMETRY_SHADER;
    glShaderStageTable[to_base(ShaderType::TESSELATION_CTRL)] = GL_TESS_CONTROL_SHADER;
    glShaderStageTable[to_base(ShaderType::TESSELATION_EVAL)] = GL_TESS_EVALUATION_SHADER;
    glShaderStageTable[to_base(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER;

    glProgramStageMask[to_base(ShaderType::VERTEX)] = GL_VERTEX_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::FRAGMENT)] = GL_FRAGMENT_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::GEOMETRY)] = GL_GEOMETRY_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::TESSELATION_CTRL)] = GL_TESS_CONTROL_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::TESSELATION_EVAL)] = GL_TESS_EVALUATION_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::COUNT)] = GL_NONE_BIT;

    glShaderStageNameTable[to_base(ShaderType::VERTEX)] = "Vertex";
    glShaderStageNameTable[to_base(ShaderType::FRAGMENT)] = "Fragment";
    glShaderStageNameTable[to_base(ShaderType::GEOMETRY)] = "Geometry";
    glShaderStageNameTable[to_base(ShaderType::TESSELATION_CTRL)] = "TessellationC";
    glShaderStageNameTable[to_base(ShaderType::TESSELATION_EVAL)] = "TessellationE";
    glShaderStageNameTable[to_base(ShaderType::COMPUTE)] = "Compute";

    glQueryTypeTable[to_base(QueryType::TIME)] = GL_TIME_ELAPSED;
    glQueryTypeTable[to_base(QueryType::PRIMITIVES_GENERATED)] = GL_PRIMITIVES_GENERATED;
    glQueryTypeTable[to_base(QueryType::GPU_TIME)] = GL_TIMESTAMP;
}

GLenum internalFormat(GFXImageFormat baseFormat, GFXDataFormat dataType, bool srgb) {
    switch (baseFormat) {
        case GFXImageFormat::RED:{
            assert(!srgb);
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return GL_R8;
                case GFXDataFormat::UNSIGNED_SHORT: return GL_R16;
                case GFXDataFormat::UNSIGNED_INT: return GL_R32UI;
                case GFXDataFormat::SIGNED_BYTE: return GL_R8I;
                case GFXDataFormat::SIGNED_SHORT: return GL_R16I;
                case GFXDataFormat::SIGNED_INT: return GL_R32I;
                case GFXDataFormat::FLOAT_16: return GL_R16F;
                case GFXDataFormat::FLOAT_32: return GL_R32F;
            };
        }break;
        case GFXImageFormat::RG: {
            assert(!srgb);
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return GL_RG8;
                case GFXDataFormat::UNSIGNED_SHORT: return GL_RG16;
                case GFXDataFormat::UNSIGNED_INT: return GL_RG32UI;
                case GFXDataFormat::SIGNED_BYTE: return GL_RG8I;
                case GFXDataFormat::SIGNED_SHORT: return GL_RG16I;
                case GFXDataFormat::SIGNED_INT: return GL_RG32I;
                case GFXDataFormat::FLOAT_16: return GL_RG16F;
                case GFXDataFormat::FLOAT_32: return GL_RG32F;
            };
        }break;
        case GFXImageFormat::BGR:
        case GFXImageFormat::RGB:
        {
            assert(!srgb || srgb == (dataType == GFXDataFormat::UNSIGNED_BYTE));
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return srgb ? GL_SRGB8 : GL_RGB8;
                case GFXDataFormat::UNSIGNED_SHORT: return GL_RGB16;
                case GFXDataFormat::UNSIGNED_INT: return GL_RGB32UI;
                case GFXDataFormat::SIGNED_BYTE: return GL_RGB8I;
                case GFXDataFormat::SIGNED_SHORT: return GL_RGB16I;
                case GFXDataFormat::SIGNED_INT: return GL_RGB32I;
                case GFXDataFormat::FLOAT_16: return GL_RGB16F;
                case GFXDataFormat::FLOAT_32: return GL_RGB32F;
            };
        }break;
        case GFXImageFormat::BGRA:
        case GFXImageFormat::RGBA:
        {
            assert(!srgb || srgb == (dataType == GFXDataFormat::UNSIGNED_BYTE));
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                case GFXDataFormat::UNSIGNED_SHORT: return GL_RGBA16;
                case GFXDataFormat::UNSIGNED_INT: return GL_RGBA32UI;
                case GFXDataFormat::SIGNED_BYTE: return GL_RGBA8I;
                case GFXDataFormat::SIGNED_SHORT: return GL_RGBA16I;
                case GFXDataFormat::SIGNED_INT: return GL_RGBA32I;
                case GFXDataFormat::FLOAT_16: return GL_RGBA16F;
                case GFXDataFormat::FLOAT_32: return GL_RGBA32F;
            };
        }break;
        case GFXImageFormat::DEPTH_COMPONENT:
        {
            switch (dataType) {
                case GFXDataFormat::SIGNED_BYTE:
                case GFXDataFormat::UNSIGNED_BYTE: return GL_DEPTH_COMPONENT16;
                case GFXDataFormat::SIGNED_SHORT:
                case GFXDataFormat::UNSIGNED_SHORT: return GL_DEPTH_COMPONENT24;
                case GFXDataFormat::SIGNED_INT:
                case GFXDataFormat::UNSIGNED_INT: return GL_DEPTH_COMPONENT32;
                case GFXDataFormat::FLOAT_16:
                case GFXDataFormat::FLOAT_32: return GL_DEPTH_COMPONENT32F;
            };
        }break;
        case GFXImageFormat::COMPRESSED_RGB_DXT1:
        {
            if (srgb) {
                return gl::GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT1:
        {
            if (srgb) {
                return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT3:
        {
            if (srgb) {
                return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT5:
        {
            if (srgb) {
                return gl::GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
    }

    return GL_NONE;
}

namespace {

void submitMultiIndirectCommand(U32 cmdOffset,
                                U32 drawCount,
                                GLenum mode,
                                GLenum internalFormat,
                                bool drawIndexed) {
    static const size_t cmdSize = sizeof(IndirectDrawCommand);
    if (drawIndexed) {
        glMultiDrawElementsIndirect(mode, internalFormat, (bufferPtr)(cmdOffset * cmdSize), drawCount, cmdSize);
    } else {
        glMultiDrawArraysIndirect(mode, (bufferPtr)(cmdOffset * cmdSize), drawCount, cmdSize);
    }
}

void submitIndirectCommand(U32 cmdOffset,
                           GLenum mode,
                           GLenum internalFormat,
                           bool drawIndexed) {
    if (drawIndexed) {
        glDrawElementsIndirect(mode, internalFormat, (bufferPtr)(cmdOffset * sizeof(IndirectDrawCommand)));
    } else {
        glDrawArraysIndirect(mode, (bufferPtr)(cmdOffset * sizeof(IndirectDrawCommand)));
    }
}

void sumitDirectCommand(const IndirectDrawCommand& cmd,
                        GLuint drawCount,
                        GLenum mode,
                        GLenum internalFormat,
                        bool drawIndexed) {

    if (drawIndexed) {
         for (GLuint i = 0; i < drawCount; ++i) {
             glDrawElementsInstanced(mode,
                                     cmd.indexCount,
                                     internalFormat,
                                     (GLvoid*)(cmd.firstIndex * (internalFormat == GL_UNSIGNED_SHORT ? sizeof(GLushort) : sizeof(GLuint))),
                                     cmd.primCount);
         }
    } else {
        for (GLuint i = 0; i < drawCount; ++i) {
            glDrawArraysInstanced(mode, cmd.firstIndex, cmd.indexCount, cmd.primCount);
        }
    }
}

void submitDirectMultiCommand(const IndirectDrawCommand& cmd,
                              U32 drawCount,
                              GLenum mode,
                              GLenum internalFormat,
                              bool drawIndexed,
                              GLsizei* countData,
                              bufferPtr indexData) {
    STUBBED("ToDo: I need a work around for this scenario. Use case: Terrain. -Ionut");
    //assert(cmd.baseInstance == 0);

    if (drawCount > 1 && cmd.primCount != 1) {
        if (drawIndexed) {
            glMultiDrawElements(mode, countData, internalFormat, (void* const*)indexData, drawCount);
        } else {
            glMultiDrawArrays(mode, (GLint*)indexData, (GLsizei*)countData, drawCount);
        }
    } else {
        sumitDirectCommand(cmd, drawCount, mode, internalFormat, drawIndexed);
    }
}

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         GLenum mode,
                         bool drawIndexed,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLsizei* countData,
                         bufferPtr indexData) {

    bool queryPrimitives = isEnabledOption(drawCommand, CmdRenderOptions::QUERY_PRIMITIVE_COUNT);
    bool querySampleCount = isEnabledOption(drawCommand, CmdRenderOptions::QUERY_SAMPLE_COUNT);
    bool querySamplePassed = isEnabledOption(drawCommand, CmdRenderOptions::QUERY_ANY_SAMPLE_RENDERED);
    
    glHardwareQueryRing* primitiveQuery = nullptr;
    glHardwareQueryRing* sampleCountQuery = nullptr;
    glHardwareQueryRing* anySamplesQuery = nullptr;

    if (queryPrimitives) {
        primitiveQuery = &GL_API::s_hardwareQueryPool->allocate();
        glBeginQuery(GL_PRIMITIVES_GENERATED, primitiveQuery->writeQuery().getID());
    }
    if (querySampleCount) {
        primitiveQuery = &GL_API::s_hardwareQueryPool->allocate();
        glBeginQuery(GL_SAMPLES_PASSED, sampleCountQuery->writeQuery().getID());
    }
    if (querySamplePassed) {
        primitiveQuery = &GL_API::s_hardwareQueryPool->allocate();
        glBeginQuery(GL_ANY_SAMPLES_PASSED, anySamplesQuery->writeQuery().getID());
    }

    if (useIndirectBuffer) {
        // Don't trust the driver to optimize the loop. Do it here so we know the cost upfront
        if (drawCommand._drawCount > 1) {
            submitMultiIndirectCommand(drawCommand._commandOffset, drawCommand._drawCount, mode, internalFormat, drawIndexed);
        } else if (drawCommand._drawCount == 1) {
            submitIndirectCommand(drawCommand._commandOffset, mode, internalFormat, drawIndexed);
        }
    } else {
        submitDirectMultiCommand(drawCommand._cmd, drawCommand._drawCount, mode, internalFormat, drawIndexed, countData, indexData);
    }

    if (queryPrimitives) {
        U64& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer->getGUID()]._primitivesGenerated;
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectui64v(primitiveQuery->readQuery().getID(),
                              GL_QUERY_RESULT,
                              &result);
    }
    if (querySampleCount) {
        U32& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer->getGUID()]._samplesPassed;
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectuiv(sampleCountQuery->readQuery().getID(),
                            GL_QUERY_RESULT,
                            &result);
    }
    if (querySamplePassed) {
        U32& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer->getGUID()]._anySamplesPassed;
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectuiv(anySamplesQuery->readQuery().getID(),
                            GL_QUERY_RESULT,
                            &result);
    }
}

};

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         bool drawIndexed,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLsizei* countData,
                         bufferPtr indexData) {
    // Process the actual draw command
    if (Config::Profile::DISABLE_DRAWS) {
        return;
    }

    DIVIDE_ASSERT(drawCommand._primitiveType != PrimitiveType::COUNT,
                  "GLUtil::submitRenderCommand error: Draw command's type is not valid!");
    
    GL_API::getStateTracker().toggleRasterization(!isEnabledOption(drawCommand, CmdRenderOptions::RENDER_NO_RASTERIZE));
    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_TESSELLATED)) {
        GL_API::getStateTracker().setPatchVertexCount(drawCommand._patchVertexCount);
    }
    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_GEOMETRY)) {
        GLenum mode = glPrimitiveTypeTable[to_base(drawCommand._primitiveType)];
        submitRenderCommand(drawCommand, mode, drawIndexed, useIndirectBuffer, internalFormat, countData, indexData);
    }
    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_WIREFRAME)) {
        submitRenderCommand(drawCommand, GL_LINE_LOOP, drawIndexed, useIndirectBuffer, internalFormat, countData, indexData);
    }
}

};  // namespace GLutil

}; //namespace Divide
