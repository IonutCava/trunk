#include "Headers/glResources.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

// We are actually importing GL specific libraries in code mainly for
// maintenance reasons
// We can easily adjust them as needed. Same thing with PhysX libs
#ifdef _DEBUG
#pragma comment(lib, "CEGUIOpenGLRenderer-0_Static_d.lib")
#pragma comment(lib, "glew32sd.lib")
#pragma comment(lib, "glbindingd.lib")
#else  //_DEBUG
#pragma comment(lib, "CEGUIOpenGLRenderer-0_Static.lib")
#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "glbinding.lib")
#endif  //_DEBUG

#include <glim.h>

namespace Divide {
namespace GLUtil {

/*-----------Object Management----*/
GLuint _invalidObjectID = GL_INVALID_INDEX;
GLFWwindow* _mainWindow = nullptr;
GLFWwindow* _loaderWindow = nullptr;

/// this may not seem very efficient (or useful) but it saves a lot of
/// single-use code scattered around further down
GLint getIntegerv(GLenum param) {
    GLint tempValue = 0;
    glGetIntegerv(param, &tempValue);
    return tempValue;
}

/// Send a shutdown request to the app on X-button clicks
void glfw_close_callback(GLFWwindow* window) {
    Application::getInstance().RequestShutdown();
}

/// Used by GLFW to inform the application if it has focus or not
void glfw_focus_callback(GLFWwindow* window, I32 focusState) {
    Application::getInstance().hasFocus(focusState != 0);
}

namespace GL_ENUM_TABLE {
GLenum
    glBlendTable[enum_to_uint_const(BlendProperty::BlendProperty_PLACEHOLDER)];
GLenum glBlendOpTable[enum_to_uint_const(
    BlendOperation::BlendOperation_PLACEHOLDER)];
GLenum glCompareFuncTable[enum_to_uint_const(
    ComparisonFunction::ComparisonFunction_PLACEHOLDER)];
GLenum glStencilOpTable[enum_to_uint_const(
    StencilOperation::StencilOperation_PLACEHOLDER)];
GLenum glCullModeTable[enum_to_uint_const(CullMode::CullMode_PLACEHOLDER)];
GLenum glFillModeTable[enum_to_uint_const(FillMode::FillMode_PLACEHOLDER)];
GLenum glTextureTypeTable[enum_to_uint_const(
    TextureType::TextureType_PLACEHOLDER)];
GLenum glImageFormatTable[enum_to_uint_const(
    GFXImageFormat::GFXImageFormat_PLACEHOLDER)];
GLenum glPrimitiveTypeTable[enum_to_uint_const(
    PrimitiveType::PrimitiveType_PLACEHOLDER)];
GLenum glDataFormat[enum_to_uint_const(GFXDataFormat::GDF_PLACEHOLDER)];
GLenum glWrapTable[enum_to_uint_const(TextureWrap::TextureWrap_PLACEHOLDER)];
GLenum glTextureFilterTable[enum_to_uint_const(
    TextureFilter::TextureFilter_PLACEHOLDER)];
NS_GLIM::GLIM_ENUM glimPrimitiveType[enum_to_uint_const(
    PrimitiveType::PrimitiveType_PLACEHOLDER)];

void fill() {
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_ZERO)] =
        GL_ZERO;
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_ONE)] =
        GL_ONE;
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_SRC_COLOR)] =
        GL_SRC_COLOR;
    glBlendTable[enum_to_uint_const(
        BlendProperty::BLEND_PROPERTY_INV_SRC_COLOR)] = GL_ONE_MINUS_SRC_COLOR;
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_SRC_ALPHA)] =
        GL_SRC_ALPHA;
    glBlendTable[enum_to_uint_const(
        BlendProperty::BLEND_PROPERTY_INV_SRC_ALPHA)] = GL_ONE_MINUS_SRC_ALPHA;
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_DEST_ALPHA)] =
        GL_DST_ALPHA;
    glBlendTable[enum_to_uint_const(
        BlendProperty::BLEND_PROPERTY_INV_DEST_ALPHA)] = GL_ONE_MINUS_DST_ALPHA;
    glBlendTable[enum_to_uint_const(BlendProperty::BLEND_PROPERTY_DEST_COLOR)] =
        GL_DST_COLOR;
    glBlendTable[enum_to_uint_const(
        BlendProperty::BLEND_PROPERTY_INV_DEST_COLOR)] = GL_ONE_MINUS_DST_COLOR;
    glBlendTable[enum_to_uint_const(
        BlendProperty::BLEND_PROPERTY_SRC_ALPHA_SAT)] = GL_SRC_ALPHA_SATURATE;

    glBlendOpTable[enum_to_uint_const(BlendOperation::BLEND_OPERATION_ADD)] =
        GL_FUNC_ADD;
    glBlendOpTable[enum_to_uint_const(
        BlendOperation::BLEND_OPERATION_SUBTRACT)] = GL_FUNC_SUBTRACT;
    glBlendOpTable[enum_to_uint_const(
        BlendOperation::BLEND_OPERATION_REV_SUBTRACT)] =
        GL_FUNC_REVERSE_SUBTRACT;
    glBlendOpTable[enum_to_uint_const(BlendOperation::BLEND_OPERATION_MIN)] =
        GL_MIN;
    glBlendOpTable[enum_to_uint_const(BlendOperation::BLEND_OPERATION_MAX)] =
        GL_MAX;

    glCompareFuncTable[enum_to_uint_const(ComparisonFunction::CMP_FUNC_NEVER)] =
        GL_NEVER;
    glCompareFuncTable[enum_to_uint_const(ComparisonFunction::CMP_FUNC_LESS)] =
        GL_LESS;
    glCompareFuncTable[enum_to_uint_const(ComparisonFunction::CMP_FUNC_EQUAL)] =
        GL_EQUAL;
    glCompareFuncTable[enum_to_uint_const(
        ComparisonFunction::CMP_FUNC_LEQUAL)] = GL_LEQUAL;
    glCompareFuncTable[enum_to_uint_const(
        ComparisonFunction::CMP_FUNC_GREATER)] = GL_GREATER;
    glCompareFuncTable[enum_to_uint_const(
        ComparisonFunction::CMP_FUNC_NEQUAL)] = GL_NOTEQUAL;
    glCompareFuncTable[enum_to_uint_const(
        ComparisonFunction::CMP_FUNC_GEQUAL)] = GL_GEQUAL;
    glCompareFuncTable[enum_to_uint_const(
        ComparisonFunction::CMP_FUNC_ALWAYS)] = GL_ALWAYS;

    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_KEEP)] = GL_KEEP;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_ZERO)] = GL_ZERO;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_REPLACE)] = GL_REPLACE;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_INCR)] = GL_INCR;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_DECR)] = GL_DECR;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_INV)] = GL_INVERT;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_INCR_WRAP)] = GL_INCR_WRAP;
    glStencilOpTable[enum_to_uint_const(
        StencilOperation::STENCIL_OPERATION_DECR_WRAP)] = GL_DECR_WRAP;

    glCullModeTable[enum_to_uint_const(CullMode::CULL_MODE_NONE)] = GL_BACK;
    glCullModeTable[enum_to_uint_const(CullMode::CULL_MODE_CW)] = GL_BACK;
    glCullModeTable[enum_to_uint_const(CullMode::CULL_MODE_CCW)] = GL_FRONT;
    glCullModeTable[enum_to_uint_const(CullMode::CULL_MODE_ALL)] =
        GL_FRONT_AND_BACK;

    glFillModeTable[enum_to_uint_const(FillMode::FILL_MODE_POINT)] = GL_POINT;
    glFillModeTable[enum_to_uint_const(FillMode::FILL_MODE_WIREFRAME)] =
        GL_LINE;
    glFillModeTable[enum_to_uint_const(FillMode::FILL_MODE_SOLID)] = GL_FILL;

    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_1D)] =
        GL_TEXTURE_1D;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_2D)] =
        GL_TEXTURE_2D;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_3D)] =
        GL_TEXTURE_3D;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_CUBE_MAP)] =
        GL_TEXTURE_CUBE_MAP;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_2D_ARRAY)] =
        GL_TEXTURE_2D_ARRAY;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_CUBE_ARRAY)] =
        GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_2D_MS)] =
        GL_TEXTURE_2D_MULTISAMPLE;
    glTextureTypeTable[enum_to_uint_const(TextureType::TEXTURE_2D_ARRAY_MS)] =
        GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

    glImageFormatTable[enum_to_uint_const(GFXImageFormat::LUMINANCE)] =
        GL_LUMINANCE;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::LUMINANCE_ALPHA)] =
        GL_LUMINANCE_ALPHA;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::LUMINANCE_ALPHA16F)] =
        gl::GL_LUMINANCE_ALPHA16F_ARB;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::LUMINANCE_ALPHA32F)] =
        gl::GL_LUMINANCE_ALPHA32F_ARB;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::INTENSITY)] =
        GL_INTENSITY;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::ALPHA)] = GL_ALPHA;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED)] = GL_RED;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED8)] = GL_R8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED16)] = GL_R16;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED16F)] = GL_R16F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED32)] = GL_R32UI;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RED32F)] = GL_R32F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::BLUE)] = GL_BLUE;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::GREEN)] = GL_GREEN;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG)] = GL_RG;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG8)] = GL_RG8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG16)] = GL_RG16;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG16F)] = GL_RG16F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG32)] = GL_RG32UI;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RG32F)] = GL_RG32F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGB)] = GL_RGB;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::BGR)] = GL_BGR;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGB8)] = GL_RGB8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::SRGB8)] = GL_SRGB8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGB8I)] = GL_RGB8I;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGB16)] = GL_RGB16;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGB16F)] = GL_RGB16F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::BGRA)] = GL_BGRA;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA)] = GL_RGBA;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA4)] = GL_RGBA4;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA8)] = GL_RGBA8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::SRGBA8)] =
        GL_SRGB8_ALPHA8;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA8I)] = GL_RGBA8I;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA16F)] =
        GL_RGBA16F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::RGBA32F)] =
        GL_RGBA32F;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::DEPTH_COMPONENT)] =
        GL_DEPTH_COMPONENT;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::DEPTH_COMPONENT16)] =
        GL_DEPTH_COMPONENT16;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::DEPTH_COMPONENT24)] =
        GL_DEPTH_COMPONENT24;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::DEPTH_COMPONENT32)] =
        GL_DEPTH_COMPONENT32;
    glImageFormatTable[enum_to_uint_const(GFXImageFormat::DEPTH_COMPONENT32F)] =
        GL_DEPTH_COMPONENT32F;

    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::API_POINTS)] =
        GL_POINTS;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::LINES)] = GL_LINES;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::LINE_LOOP)] =
        GL_LINE_LOOP;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::LINE_STRIP)] =
        GL_LINE_STRIP;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::TRIANGLES)] =
        GL_TRIANGLES;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::TRIANGLE_STRIP)] =
        GL_TRIANGLE_STRIP;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::TRIANGLE_FAN)] =
        GL_TRIANGLE_FAN;
    // glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::QUADS)] =
    // GL_QUADS; //<Deprecated
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::QUAD_STRIP)] =
        GL_QUAD_STRIP;
    glPrimitiveTypeTable[enum_to_uint_const(PrimitiveType::POLYGON)] =
        GL_POLYGON;

    glDataFormat[enum_to_uint_const(GFXDataFormat::UNSIGNED_BYTE)] =
        GL_UNSIGNED_BYTE;
    glDataFormat[enum_to_uint_const(GFXDataFormat::UNSIGNED_SHORT)] =
        GL_UNSIGNED_SHORT;
    glDataFormat[enum_to_uint_const(GFXDataFormat::UNSIGNED_INT)] =
        GL_UNSIGNED_INT;
    glDataFormat[enum_to_uint_const(GFXDataFormat::SIGNED_BYTE)] = GL_BYTE;
    glDataFormat[enum_to_uint_const(GFXDataFormat::SIGNED_SHORT)] = GL_SHORT;
    glDataFormat[enum_to_uint_const(GFXDataFormat::SIGNED_INT)] = GL_INT;
    glDataFormat[enum_to_uint_const(GFXDataFormat::FLOAT_16)] = GL_HALF_FLOAT;
    glDataFormat[enum_to_uint_const(GFXDataFormat::FLOAT_32)] = GL_FLOAT;

    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_MIRROR_REPEAT)] =
        GL_MIRRORED_REPEAT;
    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_REPEAT)] = GL_REPEAT;
    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_CLAMP)] = GL_CLAMP;
    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_CLAMP_TO_EDGE)] =
        GL_CLAMP_TO_EDGE;
    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_CLAMP_TO_BORDER)] = GL_CLAMP_TO_BORDER;
    glWrapTable[enum_to_uint_const(TextureWrap::TEXTURE_DECAL)] = GL_DECAL;

    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_LINEAR)] = GL_LINEAR;
    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_NEAREST)] = GL_NEAREST;
    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)] =
        GL_NEAREST_MIPMAP_NEAREST;
    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST)] =
        GL_LINEAR_MIPMAP_NEAREST;
    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR)] =
        GL_NEAREST_MIPMAP_LINEAR;
    glTextureFilterTable[enum_to_uint_const(TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR)] =
        GL_LINEAR_MIPMAP_LINEAR;

    glimPrimitiveType[enum_to_uint_const(PrimitiveType::API_POINTS)] = NS_GLIM::GLIM_POINTS;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::LINES)] = NS_GLIM::GLIM_LINES;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::LINE_LOOP)] = NS_GLIM::GLIM_LINE_LOOP;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::LINE_STRIP)] = NS_GLIM::GLIM_LINE_STRIP;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::TRIANGLES)] = NS_GLIM::GLIM_TRIANGLES;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::TRIANGLE_STRIP)] =
        NS_GLIM::GLIM_TRIANGLE_STRIP;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::TRIANGLE_FAN)] = NS_GLIM::GLIM_TRIANGLE_FAN;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::QUAD_STRIP)] = NS_GLIM::GLIM_QUAD_STRIP;
    glimPrimitiveType[enum_to_uint_const(PrimitiveType::POLYGON)] = NS_GLIM::GLIM_POLYGON;
}
}
}  /// GLutil

union nv_half_data {
    GLhalf bits;
    struct {
        unsigned long m : 10;
        unsigned long e : 5;
        unsigned long s : 1;
    } ieee;
};

union ieee_single {
    GLfloat f;
    struct {
        unsigned long m : 23;
        unsigned long e : 8;
        unsigned long s : 1;
    } ieee;
};

static GLfloat htof(GLhalf val) {
    nv_half_data h;
    h.bits = val;
    ieee_single sng;
    sng.ieee.s = h.ieee.s;

    // handle special cases
    if ((h.ieee.e == 0) && (h.ieee.m == 0)) {  // zero
        sng.ieee.m = 0;
        sng.ieee.e = 0;
    } else if ((h.ieee.e == 0) &&
               (h.ieee.m !=
                0)) {  // denorm -- denorm half will fit in non-denorm single
        const GLfloat half_denorm = (1.0f / 16384.0f);  // 2^-14
        GLfloat mantissa = ((GLfloat)(h.ieee.m)) / 1024.0f;
        GLfloat sgn = (h.ieee.s) ? -1.0f : 1.0f;
        sng.f = sgn * mantissa * half_denorm;
    } else if ((h.ieee.e == 31) && (h.ieee.m == 0)) {  // infinity
        sng.ieee.e = 0xff;
        sng.ieee.m = 0;
    } else if ((h.ieee.e == 31) && (h.ieee.m != 0)) {  // NaN
        sng.ieee.e = 0xff;
        sng.ieee.m = 1;
    } else {
        sng.ieee.e = h.ieee.e + 112;
        sng.ieee.m = (h.ieee.m << 13);
    }
    return sng.f;
}

static GLhalf ftoh(GLfloat val) {
    ieee_single f;
    f.f = val;
    nv_half_data h;

    h.ieee.s = f.ieee.s;

    // handle special cases
    // const GLfloat half_denorm = (1.0f/16384.0f);

    if ((f.ieee.e == 0) && (f.ieee.m == 0)) {  // zero
        h.ieee.m = 0;
        h.ieee.e = 0;
    } else if ((f.ieee.e == 0) &&
               (f.ieee.m != 0)) {  // denorm -- denorm float maps to 0 half
        h.ieee.m = 0;
        h.ieee.e = 0;
    } else if ((f.ieee.e == 0xff) && (f.ieee.m == 0)) {  // infinity
        h.ieee.m = 0;
        h.ieee.e = 31;
    } else if ((f.ieee.e == 0xff) && (f.ieee.m != 0)) {  // NaN
        h.ieee.m = 1;
        h.ieee.e = 31;
    } else {  // regular number
        GLint new_exp = f.ieee.e - 127;
        if (new_exp < -24) {  // this maps to 0
            h.ieee.m = 0;
            h.ieee.e = 0;
        }

        if (new_exp < -14) {  // this maps to a denorm
            h.ieee.e = 0;
            GLuint exp_val = (GLuint)(-14 - new_exp);  // 2^-exp_val
            switch (exp_val) {
                case 0:
                    fprintf(stderr,
                            "ftoh: logical error in denorm creation!\n");
                    h.ieee.m = 0;
                    break;
                case 1:
                    h.ieee.m = 512 + (f.ieee.m >> 14);
                    break;
                case 2:
                    h.ieee.m = 256 + (f.ieee.m >> 15);
                    break;
                case 3:
                    h.ieee.m = 128 + (f.ieee.m >> 16);
                    break;
                case 4:
                    h.ieee.m = 64 + (f.ieee.m >> 17);
                    break;
                case 5:
                    h.ieee.m = 32 + (f.ieee.m >> 18);
                    break;
                case 6:
                    h.ieee.m = 16 + (f.ieee.m >> 19);
                    break;
                case 7:
                    h.ieee.m = 8 + (f.ieee.m >> 20);
                    break;
                case 8:
                    h.ieee.m = 4 + (f.ieee.m >> 21);
                    break;
                case 9:
                    h.ieee.m = 2 + (f.ieee.m >> 22);
                    break;
                case 10:
                    h.ieee.m = 1;
                    break;
            }
        } else if (new_exp > 15) {  // map this value to infinity
            h.ieee.m = 0;
            h.ieee.e = 31;
        } else {
            h.ieee.e = new_exp + 15;
            h.ieee.m = (f.ieee.m >> 13);
        }
    }
    return h.bits;
}

static U32 VecTo_UNSIGNED_INT_2_10_10_10_REV(const vec4<U32>& values) {
    U32 returnValue = 0;
    returnValue = returnValue | (values.a << 30);
    returnValue = returnValue | (values.b << 20);
    returnValue = returnValue | (values.g << 10);
    returnValue = returnValue | (values.r << 0);

    return returnValue;
}

static I32 VecTo_INT_2_10_10_10_REV(const vec4<I32>& values) {
    I32 returnValue = 0;
    returnValue = returnValue | (values.a << 30);
    returnValue = returnValue | (values.b << 20);
    returnValue = returnValue | (values.g << 10);
    returnValue = returnValue | (values.r << 0);

    return returnValue;
}
}