#include "stdafx.h"

#include "Headers/glResources.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/GLIM/glim.h"

namespace Divide {

glObject::glObject(const glObjectType type, GFXDevice& context)
    : _type(type)
{
}

void VAOBindings::init(const U32 maxBindings) {
    _maxBindings = maxBindings;
}

VAOBindings::VAOData* VAOBindings::getVAOData(const GLuint vao) {
    VAOData* data;
    if (vao == _cachedVao) {
        data = _cachedData;
    } else {
        data = &_bindings[vao];
        _cachedData = data;
        _cachedVao = vao;
    }

    return data;
}

GLuint VAOBindings::instanceDivisor(const GLuint vao, const GLuint index) {
    VAOData* data = getVAOData(vao);

    const size_t count = data->second.size();
    if (count > 0) {
        assert(index <= count);
        return data->second[index];
    }

    assert(_maxBindings != 0);
    data->second.resize(_maxBindings);
    return data->second.front();
}

void VAOBindings::instanceDivisor(const GLuint vao, const GLuint index, const GLuint divisor) {
    VAOData* data = getVAOData(vao);

    const size_t count = data->second.size();
    assert(count > 0 && count > index);
    ACKNOWLEDGE_UNUSED(count);

    data->second[index] = divisor;
}

const VAOBindings::BufferBindingParams& VAOBindings::bindingParams(const GLuint vao, const GLuint index) {
    VAOData* data = getVAOData(vao);
   
    const size_t count = data->first.size();
    if (count > 0) {
        assert(index <= count);
        return data->first[index];
    }

    assert(_maxBindings != 0);
    data->first.resize(_maxBindings);
    return data->first.front();
}

void VAOBindings::bindingParams(const GLuint vao, const GLuint index, const BufferBindingParams& newParams) {
    VAOData* data = getVAOData(vao);

    const size_t count = data->first.size();
    assert(count > 0 && count > index);
    ACKNOWLEDGE_UNUSED(count);

    data->first[index] = newParams;
}

namespace GLUtil {

/*-----------Object Management----*/
GLuint k_invalidObjectID = GL_INVALID_INDEX;
GLuint s_lastQueryResult = GL_INVALID_INDEX;

const DisplayWindow* s_glMainRenderWindow;
thread_local SDL_GLContext s_glSecondaryContext = nullptr;
Mutex s_glSecondaryContextMutex;

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
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGB_DXT1)] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT1)] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT3)] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    glImageFormatTable[to_base(GFXImageFormat::COMPRESSED_RGBA_DXT5)] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    
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
    glShaderStageTable[to_base(ShaderType::TESSELLATION_CTRL)] = GL_TESS_CONTROL_SHADER;
    glShaderStageTable[to_base(ShaderType::TESSELLATION_EVAL)] = GL_TESS_EVALUATION_SHADER;
    glShaderStageTable[to_base(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER;

    glProgramStageMask[to_base(ShaderType::VERTEX)] = GL_VERTEX_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::FRAGMENT)] = GL_FRAGMENT_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::GEOMETRY)] = GL_GEOMETRY_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::TESSELLATION_CTRL)] = GL_TESS_CONTROL_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::TESSELLATION_EVAL)] = GL_TESS_EVALUATION_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER_BIT;
    glProgramStageMask[to_base(ShaderType::COUNT)] = GL_NONE_BIT;
}

GLenum internalFormat(const GFXImageFormat baseFormat, const GFXDataFormat dataType, const bool srgb, const bool normalized) {
    switch (baseFormat) {
        case GFXImageFormat::RED:{
            assert(!srgb);
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return normalized ? GL_R8 : GL_R8UI;
                case GFXDataFormat::UNSIGNED_SHORT: return normalized ? GL_R16 : GL_R16UI;
                case GFXDataFormat::UNSIGNED_INT: { assert(!normalized && "Format not supported"); return GL_R32UI; }
                case GFXDataFormat::SIGNED_BYTE: return normalized ? GL_R8_SNORM : GL_R8I;
                case GFXDataFormat::SIGNED_SHORT: return normalized ? GL_R16_SNORM : GL_R16I;
                case GFXDataFormat::SIGNED_INT: { assert(!normalized && "Format not supported"); return GL_R32I; }
                case GFXDataFormat::FLOAT_16: return GL_R16F;
                case GFXDataFormat::FLOAT_32: return GL_R32F;
                default: DIVIDE_UNEXPECTED_CALL();
            };
        }break;
        case GFXImageFormat::RG: {
            assert(!srgb);
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return normalized ? GL_RG8 : GL_RG8UI;
                case GFXDataFormat::UNSIGNED_SHORT: return normalized ? GL_RG16 : GL_RG16UI;
                case GFXDataFormat::UNSIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RG32UI; }
                case GFXDataFormat::SIGNED_BYTE: return normalized ? GL_RG8_SNORM : GL_RG8I;
                case GFXDataFormat::SIGNED_SHORT: return normalized ? GL_RG16_SNORM : GL_RG16I;
                case GFXDataFormat::SIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RG32I; }
                case GFXDataFormat::FLOAT_16: return GL_RG16F;
                case GFXDataFormat::FLOAT_32: return GL_RG32F;
                default: DIVIDE_UNEXPECTED_CALL();
            };
        }break;
        case GFXImageFormat::BGR:
        case GFXImageFormat::RGB:
        {
            assert(!srgb || srgb == (dataType == GFXDataFormat::UNSIGNED_BYTE && normalized));
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return normalized ? (srgb ? GL_SRGB8 : GL_RGB8) : GL_RGB8UI;
                case GFXDataFormat::UNSIGNED_SHORT: return normalized ? GL_RGB16 : GL_RGB16UI;
                case GFXDataFormat::UNSIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RGB32UI; }
                case GFXDataFormat::SIGNED_BYTE: return normalized ? GL_RGB8_SNORM : GL_RGB8I;
                case GFXDataFormat::SIGNED_SHORT: return normalized ? GL_RGB16_SNORM : GL_RGB16I;
                case GFXDataFormat::SIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RGB32I; }
                case GFXDataFormat::FLOAT_16: return GL_RGB16F;
                case GFXDataFormat::FLOAT_32: return GL_RGB32F;
                default: DIVIDE_UNEXPECTED_CALL();
            };
        }break;
        case GFXImageFormat::BGRA:
        case GFXImageFormat::RGBA:
        {
            assert(!srgb || srgb == (dataType == GFXDataFormat::UNSIGNED_BYTE && normalized));
            switch (dataType) {
                case GFXDataFormat::UNSIGNED_BYTE: return normalized ? (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8) : GL_RGBA8UI;
                case GFXDataFormat::UNSIGNED_SHORT: return normalized ? GL_RGBA16 : GL_RGBA16UI;
                case GFXDataFormat::UNSIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RGBA32UI; }
                case GFXDataFormat::SIGNED_BYTE: return normalized ? GL_RGBA8_SNORM : GL_RGBA8I;
                case GFXDataFormat::SIGNED_SHORT: return normalized ? GL_RGBA16_SNORM : GL_RGBA16I;
                case GFXDataFormat::SIGNED_INT: { assert(!normalized && "Format not supported"); return GL_RGBA32I; }
                case GFXDataFormat::FLOAT_16: return GL_RGBA16F;
                case GFXDataFormat::FLOAT_32: return GL_RGBA32F;
                default: DIVIDE_UNEXPECTED_CALL();
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
                default: DIVIDE_UNEXPECTED_CALL();
            };
        }break;
        case GFXImageFormat::COMPRESSED_RGB_DXT1:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        };
        case GFXImageFormat::COMPRESSED_RGBA_DXT1:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        };
        case GFXImageFormat::COMPRESSED_RGBA_DXT3:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        };
        case GFXImageFormat::COMPRESSED_RGBA_DXT5:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        };
        default: DIVIDE_UNEXPECTED_CALL();
    }

    return GL_NONE;
}

namespace {

void submitIndirectCommand(const IndirectDrawCommand& cmd,
                           const U32 drawCount,
                           const GLenum mode,
                           const GLenum internalFormat,
                           const bool drawIndexed,
                           const GLuint cmdBufferOffset)
{
    const size_t offset = cmdBufferOffset * sizeof(IndirectDrawCommand);
    if (drawCount == 1) {
        // We could just submit a multi-draw with a draw count of 1, but this might avoid some CPU overhead.
        // Either I or the driver has to do the count check/loop, but I can profile my own code.
        if (drawIndexed) {
            glDrawElementsIndirect(mode, internalFormat, (bufferPtr)offset);
        } else {
            // This needs a different command buffer and different IndirectDrawCommand (16byte instead of 20)
            glDrawArraysInstancedBaseInstance(mode, cmd.firstIndex, cmd.indexCount, cmd.primCount, cmd.baseInstance);
        }
    } else {
        if (drawIndexed) {
            glMultiDrawElementsIndirect(mode, internalFormat, (bufferPtr)offset, drawCount, sizeof(IndirectDrawCommand));
        } else {
            glMultiDrawArraysIndirect(mode, (bufferPtr)offset, drawCount, sizeof(IndirectDrawCommand));
        }
    }
}

void submitDirectCommand(const IndirectDrawCommand& cmd,
                         const U32 drawCount,
                         const GLenum mode,
                         const GLenum internalFormat,
                         const bool drawIndexed,
                         size_t* countData,
                         bufferPtr indexData) 
{
    if (drawCount > 1 && cmd.primCount == 1) {
        // We could just submit a multi-draw with a draw count of 1, but this might avoid some CPU overhead.
        // Either I or the driver has to do the count check/loop, but I can profile my own code.
        if (drawIndexed) {
            glMultiDrawElements(mode, reinterpret_cast<GLsizei*>(countData), internalFormat, static_cast<void* const*>(indexData), drawCount);
        } else {
            glMultiDrawArrays(mode, static_cast<GLint*>(indexData), reinterpret_cast<GLsizei*>(countData), drawCount);
        }
    } else {
        if (drawIndexed) {
            const size_t elementSize = internalFormat == GL_UNSIGNED_SHORT ? sizeof(GLushort) : sizeof(GLuint);
            for (GLuint i = 0; i < drawCount; ++i) {
                glDrawElementsInstancedBaseVertexBaseInstance(mode,
                                                              cmd.indexCount,
                                                              internalFormat,
                                                              (bufferPtr)(cmd.firstIndex * elementSize),
                                                              cmd.primCount,
                                                              cmd.baseVertex,
                                                              cmd.baseInstance);
            }
        } else {
            for (GLuint i = 0; i < drawCount; ++i) {
                glDrawArraysInstancedBaseInstance(mode,
                                                  cmd.firstIndex,
                                                  cmd.indexCount,
                                                  cmd.primCount,
                                                  cmd.baseInstance);
            }
        }
    }
}

void submitRenderCommand(const GLenum primitiveType,
                         const GenericDrawCommand& drawCommand,
                         const bool drawIndexed,
                         const bool useIndirectBuffer,
                         const GLuint cmdBufferOffset,
                         const GLenum internalFormat,
                         size_t* countData,
                         bufferPtr indexData)
{

    if (useIndirectBuffer) {
        submitIndirectCommand(drawCommand._cmd, drawCommand._drawCount, primitiveType, internalFormat, drawIndexed, cmdBufferOffset + to_U32(drawCommand._commandOffset));
    } else {
        submitDirectCommand(drawCommand._cmd, drawCommand._drawCount, primitiveType, internalFormat, drawIndexed, countData, indexData);
    }
}
};

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         const bool drawIndexed,
                         const bool useIndirectBuffer,
                         const GLuint cmdBufferOffset,
                         const GLenum internalFormat,
                         size_t* countData,
                         bufferPtr indexData)
{
    DIVIDE_ASSERT(drawCommand._primitiveType != PrimitiveType::COUNT,
                  "GLUtil::submitRenderCommand error: Draw command's type is not valid!");

    GL_API::getStateTracker().toggleRasterization(!isEnabledOption(drawCommand, CmdRenderOptions::RENDER_NO_RASTERIZE));

    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_GEOMETRY)) {
        bool runQueries = false;
        glHardwareQueryRing* primitiveQuery = nullptr;
        glHardwareQueryRing* sampleCountQuery = nullptr;
        glHardwareQueryRing* anySamplesQuery = nullptr;

        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_PRIMITIVE_COUNT)) {
            primitiveQuery = &GL_API::s_hardwareQueryPool->allocate(GL_PRIMITIVES_GENERATED);
            primitiveQuery->begin();
            runQueries = true;
        }
        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_SAMPLE_COUNT)) {
            sampleCountQuery = &GL_API::s_hardwareQueryPool->allocate(GL_SAMPLES_PASSED);
            sampleCountQuery->begin();
            runQueries = true;
        }
        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_ANY_SAMPLE_RENDERED)) {
            anySamplesQuery = &GL_API::s_hardwareQueryPool->allocate(GL_ANY_SAMPLES_PASSED);
            anySamplesQuery->begin();
            runQueries = true;
        }

        //----- DRAW ------
        const GLenum primitiveType = glPrimitiveTypeTable[to_base(drawCommand._primitiveType)];
        submitRenderCommand(primitiveType, drawCommand, drawIndexed, useIndirectBuffer, cmdBufferOffset, internalFormat, countData, indexData);
        //-----------------

        if (runQueries) {
            auto& results = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer._id];
            if (primitiveQuery != nullptr) {
               results._primitivesGenerated = to_U64(primitiveQuery->getResultNoWait());
            }
            if (sampleCountQuery != nullptr) {
                results._samplesPassed = to_U32(sampleCountQuery->getResultNoWait());
            }
            if (anySamplesQuery != nullptr) {
                results._anySamplesPassed = to_U32(anySamplesQuery->getResultNoWait());
            }
        }
    }

    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_WIREFRAME)) {
        submitRenderCommand(GL_LINE_LOOP, drawCommand, drawIndexed, useIndirectBuffer, cmdBufferOffset, internalFormat, countData, indexData);
    }
}

void glTexturePool::init(const std::array<U32, to_base(TextureType::COUNT) + 1>& poolSizes)
{
    for (U8 textureType = 0; textureType < poolSizes.size(); ++textureType) {
        const U32 poolSize = poolSizes[textureType];

        poolImpl& pool = _pools[textureType];

        pool._usageMap.reserve(poolSize);

        for (U32 i = 0; i < poolSize; ++i) {
            pool._usageMap.push_back(AtomicWrapper<State>{ State::FREE });
        }

        pool._type = static_cast<TextureType>(textureType);
        pool._handles.resize(poolSize, 0u);
        pool._lifeLeft.resize(poolSize, 0u);
        pool._tempBuffer.resize(poolSize, 0u);

        if (pool._type != TextureType::COUNT) {
            glCreateTextures(glTextureTypeTable[textureType], static_cast<GLsizei>(poolSize), pool._handles.data());
        } else {
            glGenTextures(static_cast<GLsizei>(poolSize), pool._handles.data());
        }
    }
}

void glTexturePool::onFrameEnd() {
    OPTICK_EVENT("Texture Pool: onFrameEnd");
    for (auto& pool : _pools) {
        OnFrameEndInternal(pool);
    }
}

void glTexturePool::OnFrameEndInternal(poolImpl & impl) {
    const U32 entryCount = to_U32(impl._tempBuffer.size());

    GLuint count = 0;
    for (U32 i = 0; i < entryCount; ++i) {
        if (impl._usageMap[i]._a.load() != State::CLEAN) {
            continue;
        }

        U32& lifeLeft = impl._lifeLeft[i];

        if (lifeLeft > 0) {
            lifeLeft -= 1u;
        }

        if (lifeLeft == 0) {
            impl._tempBuffer[++count] = impl._handles[i];
            GL_API::dequeueComputeMipMap(impl._handles[i]);
        }
    }

    if (count > 0) {
        glDeleteTextures(count, impl._tempBuffer.data());
        if (impl._type != TextureType::COUNT) {
            glCreateTextures(glTextureTypeTable[to_base(impl._type)], count, impl._tempBuffer.data());
        } else {
            glGenTextures(count, impl._tempBuffer.data());
        }

        U32 newIndex = 0;
        for (U32 i = 0; i < entryCount; ++i) {
            if (impl._lifeLeft[i] == 0 && impl._usageMap[i]._a.load() == State::CLEAN) {
                for (auto& [hash, handle] : impl._cache) {
                    if (handle == impl._handles[i]) {
                        handle = poolImpl::INVALID_IDX;
                        break;
                    }
                }
                impl._handles[i] = impl._tempBuffer[newIndex++];
                impl._usageMap[i]._a.store(State::FREE);
            }
        }
        std::memset(impl._tempBuffer.data(), 0, sizeof(GLuint) * count);
    }
}

void glTexturePool::destroy() {
    for (auto& pool : _pools) {

        const U32 entryCount = to_U32(pool._tempBuffer.size());
        glDeleteTextures(static_cast<GLsizei>(entryCount), pool._handles.data());
        std::memset(pool._handles.data(), 0, sizeof(GLuint) * entryCount);
        std::memset(pool._lifeLeft.data(), 0, sizeof(U32) * entryCount);

        for (auto& state : pool._usageMap) {
            state._a.store(State::CLEAN);
        }
        pool._cache.clear();
    }
}

GLuint glTexturePool::allocate(const TextureType type, const bool retry) {
    return allocate(0u, type, retry).first;
}

std::pair<GLuint, bool> glTexturePool::allocate(const size_t hash, const TextureType type, const bool retry) {
    poolImpl& impl = _pools[to_base(type)];

    if (hash != 0u) {
        U32 idx = poolImpl::INVALID_IDX;
        const auto& cacheIt = impl._cache.find(hash);
        if (cacheIt != cend(impl._cache)) {
            idx = cacheIt->second;
        }


        if (idx != poolImpl::INVALID_IDX) {
            impl._usageMap[idx]._a.store(State::USED);
            impl._lifeLeft[idx] += 1;
            return std::make_pair(impl._handles[idx], true);
        }
    }

    const U32 count = to_U32(impl._handles.size());
    for (U32 i = 0; i < count; ++i) {
        State expected = State::FREE;
        if (impl._usageMap[i]._a.compare_exchange_strong(expected, State::USED)) {
            if (hash != 0) {
                impl._cache[hash] = i;
            }

            return std::make_pair(impl._handles[i], false);
        }
    }
    
    if (!retry) {
        onFrameEnd();
        return allocate(hash, type, true);
    }

    DIVIDE_UNEXPECTED_CALL();
    return std::make_pair(0u, false);
}

void glTexturePool::deallocate(GLuint& handle, const TextureType type, const U32 frameDelay) {

    poolImpl& impl = _pools[to_base(type)];

    const U32 count = to_U32(impl._handles.size());
    for (U32 i = 0; i < count; ++i) {
        if (impl._handles[i] == handle) {
            handle = 0;
            impl._lifeLeft[i] = frameDelay;
            impl._usageMap[i]._a.store(State::CLEAN);
            return;
        }
    }
    
    DIVIDE_UNEXPECTED_CALL();
}

}  // namespace GLUtil

}  //namespace Divide
