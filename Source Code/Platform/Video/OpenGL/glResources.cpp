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
#pragma comment(lib, "glbindingd.lib")
#else  //_DEBUG
#pragma comment(lib, "CEGUIOpenGLRenderer-0_Static.lib")
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
std::array<NS_GLIM::GLIM_ENUM, to_const_uint(PrimitiveType::COUNT)>
    glimPrimitiveType;
std::array<GLenum, to_const_uint(ShaderType::COUNT)> glShaderStageTable;
std::array<stringImpl, to_const_uint(ShaderType::COUNT)>
    glShaderStageNameTable;

void fillEnumTables() {
    glBlendTable[to_const_uint(BlendProperty::ZERO)] = GL_ZERO;
    glBlendTable[to_const_uint(BlendProperty::ONE)] = GL_ONE;
    glBlendTable[to_const_uint(BlendProperty::SRC_COLOR)] = GL_SRC_COLOR;
    glBlendTable[to_const_uint(BlendProperty::INV_SRC_COLOR)] =
        GL_ONE_MINUS_SRC_COLOR;
    glBlendTable[to_const_uint(BlendProperty::SRC_ALPHA)] = GL_SRC_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::INV_SRC_ALPHA)] =
        GL_ONE_MINUS_SRC_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::DEST_ALPHA)] = GL_DST_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::INV_DEST_ALPHA)] =
        GL_ONE_MINUS_DST_ALPHA;
    glBlendTable[to_const_uint(BlendProperty::DEST_COLOR)] = GL_DST_COLOR;
    glBlendTable[to_const_uint(BlendProperty::INV_DEST_COLOR)] =
        GL_ONE_MINUS_DST_COLOR;
    glBlendTable[to_const_uint(BlendProperty::SRC_ALPHA_SAT)] =
        GL_SRC_ALPHA_SATURATE;

    glBlendOpTable[to_const_uint(BlendOperation::ADD)] = GL_FUNC_ADD;
    glBlendOpTable[to_const_uint(BlendOperation::SUBTRACT)] = GL_FUNC_SUBTRACT;
    glBlendOpTable[to_const_uint(BlendOperation::REV_SUBTRACT)] =
        GL_FUNC_REVERSE_SUBTRACT;
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
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_CUBE_MAP)] =
        GL_TEXTURE_CUBE_MAP;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_ARRAY)] =
        GL_TEXTURE_2D_ARRAY;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_CUBE_ARRAY)] =
        GL_TEXTURE_CUBE_MAP_ARRAY;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_MS)] =
        GL_TEXTURE_2D_MULTISAMPLE;
    glTextureTypeTable[to_const_uint(TextureType::TEXTURE_2D_ARRAY_MS)] =
        GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE)] = GL_LUMINANCE;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA)] =
        GL_LUMINANCE_ALPHA;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA16F)] =
        gl::GL_LUMINANCE_ALPHA16F_ARB;
    glImageFormatTable[to_const_uint(GFXImageFormat::LUMINANCE_ALPHA32F)] =
        gl::GL_LUMINANCE_ALPHA32F_ARB;
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
    glImageFormatTable[to_const_uint(GFXImageFormat::BGRA)] = GL_BGRA;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA)] = GL_RGBA;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA4)] = GL_RGBA4;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA8)] = GL_RGBA8;
    glImageFormatTable[to_const_uint(GFXImageFormat::SRGBA8)] = GL_SRGB8_ALPHA8;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA8I)] = GL_RGBA8I;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA16F)] = GL_RGBA16F;
    glImageFormatTable[to_const_uint(GFXImageFormat::RGBA32F)] = GL_RGBA32F;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT)] =
        GL_DEPTH_COMPONENT;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT16)] =
        GL_DEPTH_COMPONENT16;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT24)] =
        GL_DEPTH_COMPONENT24;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT32)] =
        GL_DEPTH_COMPONENT32;
    glImageFormatTable[to_const_uint(GFXImageFormat::DEPTH_COMPONENT32F)] =
        GL_DEPTH_COMPONENT32F;

    glPrimitiveTypeTable[to_const_uint(PrimitiveType::API_POINTS)] = GL_POINTS;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINES)] = GL_LINES;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINE_LOOP)] =
        GL_LINE_LOOP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::LINE_STRIP)] =
        GL_LINE_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLES)] =
        GL_TRIANGLES;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLE_STRIP)] =
        GL_TRIANGLE_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::TRIANGLE_FAN)] =
        GL_TRIANGLE_FAN;
    // glPrimitiveTypeTable[to_const_uint(PrimitiveType::QUADS)] =
    // GL_QUADS; //<Deprecated
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::QUAD_STRIP)] =
        GL_QUAD_STRIP;
    glPrimitiveTypeTable[to_const_uint(PrimitiveType::POLYGON)] = GL_POLYGON;

    glDataFormat[to_const_uint(GFXDataFormat::UNSIGNED_BYTE)] =
        GL_UNSIGNED_BYTE;
    glDataFormat[to_const_uint(GFXDataFormat::UNSIGNED_SHORT)] =
        GL_UNSIGNED_SHORT;
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
    glWrapTable[to_const_uint(TextureWrap::CLAMP_TO_BORDER)] =
        GL_CLAMP_TO_BORDER;
    glWrapTable[to_const_uint(TextureWrap::DECAL)] = GL_DECAL;

    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR)] = GL_LINEAR;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST)] = GL_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST_MIPMAP_NEAREST)] =
        GL_NEAREST_MIPMAP_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR_MIPMAP_NEAREST)] =
        GL_LINEAR_MIPMAP_NEAREST;
    glTextureFilterTable[to_const_uint(TextureFilter::NEAREST_MIPMAP_LINEAR)] =
        GL_NEAREST_MIPMAP_LINEAR;
    glTextureFilterTable[to_const_uint(TextureFilter::LINEAR_MIPMAP_LINEAR)] =
        GL_LINEAR_MIPMAP_LINEAR;

    glimPrimitiveType[to_const_uint(PrimitiveType::API_POINTS)] =
        NS_GLIM::GLIM_ENUM::GLIM_POINTS;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINES)] =
        NS_GLIM::GLIM_ENUM::GLIM_LINES;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINE_LOOP)] =
        NS_GLIM::GLIM_ENUM::GLIM_LINE_LOOP;
    glimPrimitiveType[to_const_uint(PrimitiveType::LINE_STRIP)] =
        NS_GLIM::GLIM_ENUM::GLIM_LINE_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLES)] =
        NS_GLIM::GLIM_ENUM::GLIM_TRIANGLES;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLE_STRIP)] =
        NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::TRIANGLE_FAN)] =
        NS_GLIM::GLIM_ENUM::GLIM_TRIANGLE_FAN;
    glimPrimitiveType[to_const_uint(PrimitiveType::QUAD_STRIP)] =
        NS_GLIM::GLIM_ENUM::GLIM_QUAD_STRIP;
    glimPrimitiveType[to_const_uint(PrimitiveType::POLYGON)] =
        NS_GLIM::GLIM_ENUM::GLIM_POLYGON;

    glShaderStageTable[to_uint(ShaderType::VERTEX)] = GL_VERTEX_SHADER;
    glShaderStageTable[to_uint(ShaderType::FRAGMENT)] = GL_FRAGMENT_SHADER;
    glShaderStageTable[to_uint(ShaderType::GEOMETRY)] = GL_GEOMETRY_SHADER;
    glShaderStageTable[to_uint(ShaderType::TESSELATION_CTRL)] =
        GL_TESS_CONTROL_SHADER;
    glShaderStageTable[to_uint(ShaderType::TESSELATION_EVAL)] =
        GL_TESS_EVALUATION_SHADER;
    glShaderStageTable[to_uint(ShaderType::COMPUTE)] = GL_COMPUTE_SHADER;

    glShaderStageNameTable[to_uint(ShaderType::VERTEX)] = "Vertex";
    glShaderStageNameTable[to_uint(ShaderType::FRAGMENT)] = "Fragment";
    glShaderStageNameTable[to_uint(ShaderType::GEOMETRY)] = "Geometry";
    glShaderStageNameTable[to_uint(ShaderType::TESSELATION_CTRL)] =
        "TessellationC";
    glShaderStageNameTable[to_uint(ShaderType::TESSELATION_EVAL)] =
        "TessellationE";
    glShaderStageNameTable[to_uint(ShaderType::COMPUTE)] = "Compute";
}


namespace DSAWrapper {

    // Textures
    void dsaCreateTextures(GLenum target, GLsizei count, GLuint* textures) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glCreateTextures(target, count, textures);
            return;
        }
#endif

        glGenTextures(count, textures);
    }

    void dsaGenerateTextureMipmap(GLuint texture, GLenum target) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glGenerateTextureMipmap(texture);
                return;
            }
#endif

            glGenerateTextureMipmapEXT(texture, target);
            return;
        }
        GL_API::bindTexture(0, texture, target);
        glGenerateMipmap(target);
    }

    void dsaTextureStorage(GLuint texture, GLenum target, GLsizei levels,
        GLenum internalformat, GLsizei width,
        GLsizei height, GLsizei depth) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            if (height == -1 && depth == -1) {
                glTextureStorage1D(texture, levels, internalformat, width);
            }
            else if (depth == -1) {
                glTextureStorage2D(texture, levels, internalformat, width, height);
            }
            else {
                glTextureStorage3D(texture, levels, internalformat, width, height,
                    depth);
            }
            return;
        }
#endif

        if (height == -1 && depth == -1) {
            glext::glTextureStorage1DEXT(texture, target, levels, internalformat,
                width);
        }
        else if (depth == -1) {
            glext::glTextureStorage2DEXT(texture, target, levels, internalformat,
                width, height);
        }
        else {
            glext::glTextureStorage3DEXT(texture, target, levels, internalformat,
                width, height, depth);
        }
    }

    void dsaTextureStorageMultisample(GLuint texture, GLenum target,
        GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height,
        GLsizei depth,
        GLboolean fixedsamplelocations) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            if (depth == -1) {
                glTextureStorage2DMultisample(texture, samples, internalformat,
                    width, height, fixedsamplelocations);
            }
            else {
                glTextureStorage3DMultisample(texture, samples, internalformat,
                    width, height, depth,
                    fixedsamplelocations);
            }
            return;
        }
#endif
        if (depth == -1) {
            glext::glTextureStorage2DMultisampleEXT(texture, target, samples,
                internalformat, width, height,
                fixedsamplelocations);
        }
        else {
            glext::glTextureStorage3DMultisampleEXT(texture, target, samples,
                internalformat, width, height,
                depth, fixedsamplelocations);
        }
    }

    void dsaTextureSubImage(GLuint texture, GLenum target, GLint level,
        GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth,
        GLenum format, GLenum type,
        const bufferPtr pixels) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            if (height == -1 && depth == -1) {
                glTextureSubImage1D(texture, level, xoffset, width, format, type,
                    pixels);
            }
            else if (depth == -1) {
                glTextureSubImage2D(texture, level, xoffset, yoffset, width, height,
                    format, type, pixels);
            }
            else {
                glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset,
                    width, height, depth, format, type, pixels);
            }
            return;
        }
#endif

        if (height == -1 && depth == -1) {
            glext::glTextureSubImage1DEXT(texture, target, level, xoffset, width,
                format, type, pixels);
        }
        else if (depth == -1 || target == GL_TEXTURE_CUBE_MAP) {
            glext::glTextureSubImage2DEXT(
                texture,
                target == GL_TEXTURE_CUBE_MAP
                ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLuint>(zoffset)
                : target,
                level, xoffset, yoffset, width, height, format, type, pixels);
        }
        else {
            glext::glTextureSubImage3DEXT(texture, target, level, xoffset, yoffset,
                zoffset, width, height, depth, format,
                type, pixels);
        }
    }

    void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname,
        GLfloat param) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glTextureParameterf(texture, pname, param);
                return;
            }
#endif
            glext::glTextureParameterfEXT(texture, target, pname, param);
            return;
        }
        GL_API::bindTexture(0, texture, target);
        glTexParameterf(target, pname, param);
    }

    void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname,
        GLint param) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glTextureParameteri(texture, pname, param);
                return;
    }
#endif
            glext::glTextureParameteriEXT(texture, target, pname, param);
            return;
}
        GL_API::bindTexture(0, texture, target);
        glTexParameteri(target, pname, param);
    }

    // Samplers
    void dsaCreateSamplers(GLsizei count, GLuint* samplers) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glCreateSamplers(count, samplers);
            return;
        }
#endif

        glGenSamplers(count, samplers);
    }

    // VAO
    void dsaCreateVertexArrays(GLuint count, GLuint* arrays) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glCreateVertexArrays(count, arrays);
            return;
        }
#endif

        glGenVertexArrays(count, arrays);
    }

    // Buffers
    void dsaCreateBuffers(GLuint count, GLuint* buffers) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glCreateBuffers(count, buffers);
            return;
        }
#endif


        glGenBuffers(count, buffers);
    }

    void dsaNamedBufferSubData(GLuint buffer, GLintptr offset,
        GLsizeiptr size, const bufferPtr data) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glNamedBufferSubData(buffer, offset, size, data);
            return;
        }
#endif

        glext::glNamedBufferSubDataEXT(buffer, offset, size, data);
    }

    void dsaNamedBufferData(GLuint buffer, GLsizeiptr size,
        const bufferPtr data, GLenum usage) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glNamedBufferData(buffer, size, data, usage);
            return;
        }
#endif

        glext::glNamedBufferDataEXT(buffer, size, data, usage);
    }

    GLboolean dsaUnmapNamedBuffer(GLuint buffer) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            return glUnmapNamedBuffer(buffer);
        }
#endif

        return glext::glUnmapNamedBufferEXT(buffer);
    }

    void dsaNamedBufferStorage(GLuint buffer, GLsizeiptr size,
        const bufferPtr data,
        BufferStorageMask flags) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glNamedBufferStorage(buffer, size, data, flags);
            return;
        }
#endif

        glext::glNamedBufferStorageEXT(buffer, size, data, flags);
    }

    bufferPtr dsaMapNamedBufferRange(GLuint buffer, GLintptr offset,
        GLsizeiptr length,
        BufferAccessMask access) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            return glMapNamedBufferRange(buffer, offset, length, access);
        }
#endif

        return glext::glMapNamedBufferRangeEXT(buffer, offset, length, access);
    }

    // Framebuffers
    void dsaCreateFramebuffers(GLuint count, GLuint* framebuffers) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glCreateFramebuffers(count, framebuffers);
            return;
        }
#endif

        glGenFramebuffers(count, framebuffers);
    }

    void dsaNamedFramebufferTexture(GLuint framebuffer, GLenum attachment,
        GLuint texture, GLint level) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glNamedFramebufferTexture(framebuffer, attachment, texture, level);
                return;
    }
#endif

            glNamedFramebufferTextureEXT(framebuffer, attachment, texture, level);
            return;
}
        GLuint oldBuffer;
        GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture, level);
        GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
    }

    void dsaNamedFramebufferTextureLayer(GLuint framebuffer,
        GLenum attachment, GLuint texture,
        GLint level, GLint layer) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level,
                    layer);
                return;
    }
#endif

            glNamedFramebufferTextureLayerEXT(framebuffer, attachment, texture, level,
                layer);
            return;
}
        GLuint oldBuffer;
        GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture, level, layer);
        GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
    }

    void dsaNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {
        if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
            if (!GL_USE_DSA_EXTENSION) {
                glNamedFramebufferDrawBuffer(framebuffer, buf);
                return;
    }
#endif
            glext::glFramebufferDrawBufferEXT(framebuffer, buf);
            return;
    }

    GLuint oldBuffer;
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
    glDrawBuffer(buf);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
}


void dsaNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs) {
    if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
            return;
        }
#endif
        glext::glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
        return;
    }
    GLuint oldBuffer;
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
    glDrawBuffers(n, bufs);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
}

void dsaNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {
    if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            glNamedFramebufferReadBuffer(framebuffer, src);
            return;
        }
#endif
        glext::glFramebufferReadBufferEXT(framebuffer, src);
        return;
    }

    GLuint oldBuffer;
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
    glReadBuffer(src);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
}

GLenum dsaCheckNamedFramebufferStatus(GLuint framebuffer,
    GLenum target) {
    if (GL_USE_DSA) {
#ifdef GL_VERSION_4_5
        if (!GL_USE_DSA_EXTENSION) {
            return glCheckNamedFramebufferStatus(framebuffer, target);
        }
#endif

        return glCheckNamedFramebufferStatusEXT(framebuffer, target);
    }

    GLuint oldBuffer;
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, framebuffer, oldBuffer);
    GLenum status = glCheckFramebufferStatus(target);
    GL_API::setActiveFB(Framebuffer::FramebufferUsage::FB_READ_WRITE, oldBuffer);
    return status;
}

};  // namespace DSAWrapper
};  // namespace GLutil

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

//Use this: https://github.com/preshing/cpp11-on-multicore/blob/master/common/bitfield.h
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

}; //namespace Divide