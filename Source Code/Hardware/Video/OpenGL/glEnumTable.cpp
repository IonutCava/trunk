#include "Headers/glResources.h"
#include "Headers/glEnumTable.h"

GLenum glBlendTable[BlendProperty_PLACEHOLDER];
GLenum glBlendOpTable[BlendOperation_PLACEHOLDER];
GLenum glCompareFuncTable[ComparisonFunction_PLACEHOLDER];
GLenum glStencilOpTable[StencilOperation_PLACEHOLDER];
GLenum glCullModeTable[CullMode_PLACEHOLDER];
GLenum glFillModeTable[FillMode_PLACEHOLDER];
GLenum glTextureTypeTable[TextureType_PLACEHOLDER];
GLenum glImageFormatTable[IMAGE_FORMAT_PLACEHOLDER];
GLenum glPrimitiveTypeTable[PrimitiveType_PLACEHOLDER];
GLenum glDataFormat[GDF_PLACEHOLDER];
GLuint glWrapTable[TextureWrap_PLACEHOLDER];
GLuint glTextureFilterTable[TextureFilter_PLACEHOLDER];

namespace GL_ENUM_TABLE {
    void fill(){
       glBlendTable[BLEND_PROPERTY_ZERO] = GL_ZERO;
       glBlendTable[BLEND_PROPERTY_ONE] = GL_ONE;
       glBlendTable[BLEND_PROPERTY_SRC_COLOR] = GL_SRC_COLOR;
       glBlendTable[BLEND_PROPERTY_INV_SRC_COLOR] = GL_ONE_MINUS_SRC_COLOR;
       glBlendTable[BLEND_PROPERTY_SRC_ALPHA] = GL_SRC_ALPHA;
       glBlendTable[BLEND_PROPERTY_INV_SRC_ALPHA] = GL_ONE_MINUS_SRC_ALPHA;
       glBlendTable[BLEND_PROPERTY_DEST_ALPHA] = GL_DST_ALPHA;
       glBlendTable[BLEND_PROPERTY_INV_DEST_ALPHA] = GL_ONE_MINUS_DST_ALPHA;
       glBlendTable[BLEND_PROPERTY_DEST_COLOR] = GL_DST_COLOR;
       glBlendTable[BLEND_PROPERTY_INV_DEST_COLOR] = GL_ONE_MINUS_DST_COLOR;
       glBlendTable[BLEND_PROPERTY_SRC_ALPHA_SAT] = GL_SRC_ALPHA_SATURATE;

       glBlendOpTable[BLEND_OPERATION_ADD] = GL_FUNC_ADD;
       glBlendOpTable[BLEND_OPERATION_SUBTRACT] = GL_FUNC_SUBTRACT;
       glBlendOpTable[BLEND_OPERATION_REV_SUBTRACT] = GL_FUNC_REVERSE_SUBTRACT;
       glBlendOpTable[BLEND_OPERATION_MIN] = GL_MIN;
       glBlendOpTable[BLEND_OPERATION_MAX] = GL_MAX;

       glCompareFuncTable[CMP_FUNC_NEVER] = GL_NEVER;
       glCompareFuncTable[CMP_FUNC_LESS] = GL_LESS;
       glCompareFuncTable[CMP_FUNC_EQUAL] = GL_EQUAL;
       glCompareFuncTable[CMP_FUNC_LEQUAL] = GL_LEQUAL;
       glCompareFuncTable[CMP_FUNC_GREATER] = GL_GREATER;
       glCompareFuncTable[CMP_FUNC_NEQUAL] = GL_NOTEQUAL;
       glCompareFuncTable[CMP_FUNC_GEQUAL] = GL_GEQUAL;
       glCompareFuncTable[CMP_FUNC_ALWAYS] = GL_ALWAYS;

       glStencilOpTable[STENCIL_OPERATION_KEEP] = GL_KEEP;
       glStencilOpTable[STENCIL_OPERATION_ZERO] = GL_ZERO;
       glStencilOpTable[STENCIL_OPERATION_REPLACE] = GL_REPLACE;
       glStencilOpTable[STENCIL_OPERATION_INCR] = GL_INCR;
       glStencilOpTable[STENCIL_OPERATION_DECR] = GL_DECR;
       glStencilOpTable[STENCIL_OPERATION_INV]  = GL_INVERT;
       glStencilOpTable[STENCIL_OPERATION_INCR_WRAP] = GL_INCR_WRAP;
       glStencilOpTable[STENCIL_OPERATION_DECR_WRAP] = GL_DECR_WRAP;

       glCullModeTable[CULL_MODE_NONE] = GL_BACK;
       glCullModeTable[CULL_MODE_CW]   = GL_BACK;
       glCullModeTable[CULL_MODE_CCW]  = GL_FRONT;
       glCullModeTable[CULL_MODE_ALL]  = GL_FRONT_AND_BACK;

       glFillModeTable[FILL_MODE_POINT]     = GL_POINT;
       glFillModeTable[FILL_MODE_WIREFRAME] = GL_LINE;
       glFillModeTable[FILL_MODE_SOLID]     = GL_FILL;

       glTextureTypeTable[TEXTURE_1D] = GL_TEXTURE_1D;
       glTextureTypeTable[TEXTURE_2D] = GL_TEXTURE_2D;
       glTextureTypeTable[TEXTURE_3D] = GL_TEXTURE_3D;
       glTextureTypeTable[TEXTURE_CUBE_MAP] = GL_TEXTURE_CUBE_MAP;
       glTextureTypeTable[TEXTURE_2D_ARRAY] = GL_TEXTURE_2D_ARRAY_EXT;
       glTextureTypeTable[TEXTURE_CUBE_ARRAY] = GL_TEXTURE_CUBE_MAP_ARRAY;
       glTextureTypeTable[TEXTURE_2D_MS] = GL_TEXTURE_2D_MULTISAMPLE;
       glTextureTypeTable[TEXTURE_2D_ARRAY_MS] = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

       glImageFormatTable[LUMINANCE] = GL_LUMINANCE;
       glImageFormatTable[LUMINANCE_ALPHA] = GL_LUMINANCE_ALPHA;
       glImageFormatTable[LUMINANCE_ALPHA16F] = GL_LUMINANCE_ALPHA16F_ARB;
       glImageFormatTable[LUMINANCE_ALPHA32F] = GL_LUMINANCE_ALPHA32F_ARB;
       glImageFormatTable[INTENSITY] = GL_INTENSITY;
       glImageFormatTable[ALPHA] = GL_ALPHA;
       glImageFormatTable[RED] = GL_RED;
       glImageFormatTable[RED8] = GL_R8;
       glImageFormatTable[RED16] = GL_R16;
       glImageFormatTable[RED16F] = GL_R16F;
       glImageFormatTable[RED32] = GL_R32UI;
       glImageFormatTable[RED32F ] = GL_R32F;
       glImageFormatTable[BLUE] = GL_BLUE;
       glImageFormatTable[GREEN] = GL_GREEN;
       glImageFormatTable[RG] = GL_RG;
       glImageFormatTable[RG8] = GL_RG8;
       glImageFormatTable[RG16] = GL_RG16;
       glImageFormatTable[RG16F] = GL_RG16F;
       glImageFormatTable[RG32]  = GL_RG32UI;
       glImageFormatTable[RG32F] = GL_RG32F;
       glImageFormatTable[RGB] = GL_RGB;
       glImageFormatTable[BGR] = GL_BGR;
       glImageFormatTable[RGB8] = GL_RGB8;
       glImageFormatTable[RGB8I] = GL_RGB8I;
       glImageFormatTable[RGB16] = GL_RGB16;
       glImageFormatTable[RGB16F] = GL_RGB16F;
       glImageFormatTable[BGRA] = GL_BGRA;
       glImageFormatTable[RGBA] = GL_RGBA;
       glImageFormatTable[RGBA4] = GL_RGBA4;
       glImageFormatTable[RGBA8] = GL_RGBA8;
       glImageFormatTable[RGBA8I] = GL_RGBA8I;
       glImageFormatTable[RGBA16F] = GL_RGBA16F;
       glImageFormatTable[RGBA32F] = GL_RGBA32F;
       glImageFormatTable[DEPTH_COMPONENT] = GL_DEPTH_COMPONENT;
       glImageFormatTable[DEPTH_COMPONENT16] = GL_DEPTH_COMPONENT16;
       glImageFormatTable[DEPTH_COMPONENT24] = GL_DEPTH_COMPONENT24;
       glImageFormatTable[DEPTH_COMPONENT32] = GL_DEPTH_COMPONENT32;
       glImageFormatTable[DEPTH_COMPONENT32F] = GL_DEPTH_COMPONENT32F;

       glPrimitiveTypeTable[API_POINTS] = GL_POINTS;
       glPrimitiveTypeTable[LINES] = GL_LINES;
       glPrimitiveTypeTable[LINE_LOOP] = GL_LINE_LOOP;
       glPrimitiveTypeTable[LINE_STRIP] = GL_LINE_STRIP;
       glPrimitiveTypeTable[TRIANGLES] = GL_TRIANGLES;
       glPrimitiveTypeTable[TRIANGLE_STRIP] = GL_TRIANGLE_STRIP;
       glPrimitiveTypeTable[TRIANGLE_FAN] = GL_TRIANGLE_FAN;
       //glPrimitiveTypeTable[QUADS] = GL_QUADS; //<Deprecated
       glPrimitiveTypeTable[QUAD_STRIP] = GL_QUAD_STRIP;
       glPrimitiveTypeTable[POLYGON] = GL_POLYGON;

       glDataFormat[UNSIGNED_BYTE] = GL_UNSIGNED_BYTE;
       glDataFormat[UNSIGNED_SHORT] = GL_UNSIGNED_SHORT;
       glDataFormat[UNSIGNED_INT] = GL_UNSIGNED_INT;
       glDataFormat[SIGNED_BYTE] = GL_BYTE;
       glDataFormat[SIGNED_SHORT] = GL_SHORT;
       glDataFormat[SIGNED_INT] = GL_INT;
       glDataFormat[FLOAT_16] = GL_HALF_FLOAT;
       glDataFormat[FLOAT_32] = GL_FLOAT;

       glWrapTable[TEXTURE_MIRROR_REPEAT] = GL_MIRRORED_REPEAT;
       glWrapTable[TEXTURE_REPEAT] = GL_REPEAT;
       glWrapTable[TEXTURE_CLAMP] = GL_CLAMP;
       glWrapTable[TEXTURE_CLAMP_TO_EDGE] = GL_CLAMP_TO_EDGE;
       glWrapTable[TEXTURE_CLAMP_TO_BORDER] =  GL_CLAMP_TO_BORDER;
       glWrapTable[TEXTURE_DECAL] = GL_DECAL;

       glTextureFilterTable[TEXTURE_FILTER_LINEAR] = GL_LINEAR;
       glTextureFilterTable[TEXTURE_FILTER_NEAREST] = GL_NEAREST;
       glTextureFilterTable[TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST] = GL_NEAREST_MIPMAP_NEAREST;
       glTextureFilterTable[TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST] = GL_LINEAR_MIPMAP_NEAREST;
       glTextureFilterTable[TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR] = GL_NEAREST_MIPMAP_LINEAR;
       glTextureFilterTable[TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR] = GL_LINEAR_MIPMAP_LINEAR;
    }
}