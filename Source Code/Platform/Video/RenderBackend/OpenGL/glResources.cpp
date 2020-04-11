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
    : _maxBindings(0),
      _cachedData(nullptr),
      _cachedVao(0)
{
}

VAOBindings::~VAOBindings()
{
}

void VAOBindings::init(U32 maxBindings) {
    _maxBindings = maxBindings;
}

const VAOBindings::BufferBindingParams& VAOBindings::bindingParams(GLuint vao, GLuint index) {
    VAOBufferData* data = nullptr;
    if (vao == _cachedVao) {
        data = _cachedData;
    } else {
        data = &_bindings[vao];
        _cachedData = data;
        _cachedVao = vao;
    }
    
    const vec_size count = data->size();
    if (count > 0) {
        assert(index <= count);
        return (*data)[index];
    }

    assert(_maxBindings != 0);
    data->resize(_maxBindings);
    return data->front();
}

void VAOBindings::bindingParams(GLuint vao, GLuint index, const BufferBindingParams& newParams) {
    VAOBufferData* data = nullptr;
    if (vao == _cachedVao) {
        data = _cachedData;
    } else {
        data = &_bindings[vao];
        _cachedData = data;
        _cachedVao = vao;
    }

    const vec_size count = data->size();
    assert(count > 0 && count > index);
    ACKNOWLEDGE_UNUSED(count);

    (*data)[index] = newParams;
}

namespace GLUtil {

/*-----------Object Management----*/
GLuint k_invalidObjectID = GL_INVALID_INDEX;
GLuint s_lastQueryResult = GL_INVALID_INDEX;

const DisplayWindow* s_glMainRenderWindow;
thread_local SDL_GLContext s_glSecondaryContext = nullptr;
std::mutex s_driverLock;
std::mutex s_glSecondaryContextMutex;

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
std::array<Str16, to_base(ShaderType::COUNT)> glShaderStageNameTable;
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

    glShaderStageNameTable[to_base(ShaderType::VERTEX)] = "Vertex";
    glShaderStageNameTable[to_base(ShaderType::FRAGMENT)] = "Fragment";
    glShaderStageNameTable[to_base(ShaderType::GEOMETRY)] = "Geometry";
    glShaderStageNameTable[to_base(ShaderType::TESSELLATION_CTRL)] = "TessellationC";
    glShaderStageNameTable[to_base(ShaderType::TESSELLATION_EVAL)] = "TessellationE";
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
                return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT1:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT3:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
            }

            return glImageFormatTable[to_base(baseFormat)];
        }break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT5:
        {
            if (srgb) {
                return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
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
    const size_t offset = cmdOffset * sizeof(IndirectDrawCommand);
    if (drawIndexed) {
        glMultiDrawElementsIndirect(mode, internalFormat, (bufferPtr)offset, drawCount, sizeof(IndirectDrawCommand));
    } else {
        glMultiDrawArraysIndirect(mode, (bufferPtr)offset, drawCount, sizeof(IndirectDrawCommand));
    }
}

void submitIndirectCommand(const IndirectDrawCommand& cmd,
                           U32 cmdOffset,
                           GLenum mode,
                           GLenum internalFormat,
                           bool drawIndexed) {
    if (drawIndexed) {
        glDrawElementsIndirect(mode, internalFormat, (bufferPtr)(cmdOffset * sizeof(IndirectDrawCommand)));
    } else {
        // This needs a different command buffer and different IndirectDrawCommand (16byte instead of 20)
        glDrawArraysInstancedBaseInstance(mode, cmd.firstIndex, cmd.indexCount, cmd.primCount, cmd.baseInstance);
    }
}

void sumitDirectCommand(const IndirectDrawCommand& cmd,
                        GLuint drawCount,
                        GLenum mode,
                        GLenum internalFormat,
                        bool drawIndexed) {

    if (drawIndexed) {
        const size_t elementSize = (internalFormat == GL_UNSIGNED_SHORT ? sizeof(GLushort) : sizeof(GLuint));

         for (GLuint i = 0; i < drawCount; ++i) {
             glDrawElementsInstancedBaseVertexBaseInstance(
                 mode,
                 cmd.indexCount,
                 internalFormat,
                 (bufferPtr)(cmd.firstIndex * elementSize),
                 cmd.primCount,
                 cmd.baseVertex,
                 cmd.baseInstance);
         }
    } else {
        for (GLuint i = 0; i < drawCount; ++i) {
            glDrawArraysInstancedBaseInstance(
                mode,
                cmd.firstIndex,
                cmd.indexCount,
                cmd.primCount,
                cmd.baseInstance);
        }
    }
}

void submitDirectMultiCommand(const IndirectDrawCommand& cmd,
                              U32 drawCount,
                              GLenum mode,
                              GLenum internalFormat,
                              bool drawIndexed,
                              size_t* countData,
                              bufferPtr indexData) {
    if (drawCount > 1 && cmd.primCount == 1) {
        if (drawIndexed) {
            glMultiDrawElements(mode, (GLsizei*)countData, internalFormat, (void* const*)indexData, drawCount);
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
                         GLuint cmdBufferOffset,
                         GLenum internalFormat,
                         size_t* countData,
                         bufferPtr indexData) {

    if (useIndirectBuffer) {
        // Don't trust the driver to optimize the loop. Do it here so we know the cost upfront
        if (drawCommand._drawCount > 1) {
            submitMultiIndirectCommand(to_U32(drawCommand._commandOffset) + cmdBufferOffset, drawCommand._drawCount, mode, internalFormat, drawIndexed);
        } else if (drawCommand._drawCount == 1) {
            submitIndirectCommand(drawCommand._cmd, to_U32(drawCommand._commandOffset) + cmdBufferOffset, mode, internalFormat, drawIndexed);
        }
    } else {
        submitDirectMultiCommand(drawCommand._cmd, drawCommand._drawCount, mode, internalFormat, drawIndexed, countData, indexData);
    }
}

};

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         bool drawIndexed,
                         bool useIndirectBuffer,
                         GLuint cmdBufferOffset,
                         GLenum internalFormat,
                         size_t* countData,
                         bufferPtr indexData)
{
    DIVIDE_ASSERT(drawCommand._primitiveType != PrimitiveType::COUNT,
                  "GLUtil::submitRenderCommand error: Draw command's type is not valid!");

    GLStateTracker& stateTracker = GL_API::getStateTracker();

    stateTracker.toggleRasterization(!isEnabledOption(drawCommand, CmdRenderOptions::RENDER_NO_RASTERIZE));

    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_GEOMETRY)) {
        glHardwareQueryRing* primitiveQuery = nullptr;
        glHardwareQueryRing* sampleCountQuery = nullptr;
        glHardwareQueryRing* anySamplesQuery = nullptr;

        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_PRIMITIVE_COUNT)) {
            primitiveQuery = &GL_API::s_hardwareQueryPool->allocate(GL_PRIMITIVES_GENERATED);
            primitiveQuery->begin();
        }
        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_SAMPLE_COUNT)) {
            sampleCountQuery = &GL_API::s_hardwareQueryPool->allocate(GL_SAMPLES_PASSED);
            sampleCountQuery->begin();
        }
        if (isEnabledOption(drawCommand, CmdRenderOptions::QUERY_ANY_SAMPLE_RENDERED)) {
            anySamplesQuery = &GL_API::s_hardwareQueryPool->allocate(GL_ANY_SAMPLES_PASSED);
            anySamplesQuery->begin();
        }

        //----- DRAW ------
        submitRenderCommand(drawCommand, glPrimitiveTypeTable[to_base(drawCommand._primitiveType)], drawIndexed, useIndirectBuffer, cmdBufferOffset, internalFormat, countData, indexData);
        //-----------------

        if (primitiveQuery != nullptr) {
            U64& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer._id]._primitivesGenerated;
            result = to_U64(primitiveQuery->getResultNoWait());
        }
        if (sampleCountQuery != nullptr) {
            U32& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer._id]._samplesPassed;
            result = to_U32(sampleCountQuery->getResultNoWait());
        }
        if (anySamplesQuery != nullptr) {
            U32& result = GenericDrawCommandResults::g_queryResults[drawCommand._sourceBuffer._id]._anySamplesPassed;
            result = to_U32(anySamplesQuery->getResultNoWait());
        }
    }

    if (isEnabledOption(drawCommand, CmdRenderOptions::RENDER_WIREFRAME)) {
        submitRenderCommand(drawCommand, GL_LINE_LOOP, drawIndexed, useIndirectBuffer, cmdBufferOffset, internalFormat, countData, indexData);
    }
}


void glTexturePool::init(const vectorEASTL<std::pair<GLenum, U32>>& poolSizes)
{
    _types.reserve(poolSizes.size());
    //_pools.reserve(poolSizes.size());

    for (const auto& it : poolSizes) {
        poolImpl pool;
        pool._usageMap.reserve(it.second); 
        for (U32 i = 0; i < it.second; ++i) {
            pool._usageMap.push_back({ State::FREE });
        }
        pool._type = it.first;
        pool._handles.resize(it.second, 0u);
        pool._lifeLeft.resize(it.second, 0u);
        pool._tempBuffer.resize(it.second, 0u);
        if (it.first != GL_NONE) {
            glCreateTextures(it.first, (GLsizei)it.second, pool._handles.data());
        } else {
            glGenTextures((GLsizei)it.second, pool._handles.data());
        }
        _types.push_back(it.first);

        hashAlg::insert(_pools, it.first, std::move(pool));
    }
}

void glTexturePool::onFrameEnd() {
    for (auto& it : _pools) {
        onFrameEndInternal(it.second);
    }
}

void glTexturePool::onFrameEndInternal(poolImpl & impl) {
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
            impl._tempBuffer[count++] = impl._handles[i];
        }
    }

    if (count > 0) {
        glDeleteTextures(count, impl._tempBuffer.data());
        if (impl._type != GL_NONE) {
            glCreateTextures(impl._type, count, impl._tempBuffer.data());
        } else {
            glGenTextures(count, impl._tempBuffer.data());
        }

        U32 newIndex = 0;
        for (U32 i = 0; i < entryCount; ++i) {
            if (impl._lifeLeft[i] == 0 && impl._usageMap[i]._a.load() == State::CLEAN) {
                for (auto& [hash, handle] : impl._cache) {
                    if (handle == impl._handles[i]) {
                        handle = poolImpl::INVALID_IDX;
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
    for (auto& [type, impl] : _pools) {

        const U32 entryCount = to_U32(impl._tempBuffer.size());
        glDeleteTextures((GLsizei)entryCount, impl._handles.data());
        std::memset(impl._handles.data(), 0, sizeof(GLuint) * entryCount);
        std::memset(impl._lifeLeft.data(), 0, sizeof(U32) * entryCount);

        for (auto& state : impl._usageMap) {
            state._a.store(State::CLEAN);
        }
        impl._cache.clear();
    }
}

GLuint glTexturePool::allocate(GLenum type, bool retry) {
    return allocate(0u, type, retry).first;
}

std::pair<GLuint, bool> glTexturePool::allocate(size_t hash, GLenum type, bool retry) {
    auto it = _pools.find(type);
    assert(it != _pools.cend());

    poolImpl& impl = it->second;

    U32 idx = poolImpl::INVALID_IDX;
    if (hash != 0u) {
        const auto& cacheIt = impl._cache.find(hash);
        if (cacheIt != eastl::cend(impl._cache)) {
            idx = cacheIt->second;
        }
    }
    if (idx != poolImpl::INVALID_IDX) {
        impl._usageMap[idx]._a.store(State::USED);
        impl._lifeLeft[idx] += 1;
        return std::make_pair(impl._handles[idx], true);
    } else {
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
    }
    if (!retry) {
        onFrameEnd();
        return allocate(hash, type, true);
    }

    DIVIDE_UNEXPECTED_CALL();
    return std::make_pair(0u, false);
}

void glTexturePool::deallocate(GLuint& handle, GLenum type, U32 frameDelay) {
    const auto it = _pools.find(type);
    assert(it != _pools.cend());

    poolImpl& impl = it->second;

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

bool glTexturePool::hasPool(GLenum type) const {
    return _pools.find(type) != eastl::cend(_pools);
}

};  // namespace GLutil

}; //namespace Divide
