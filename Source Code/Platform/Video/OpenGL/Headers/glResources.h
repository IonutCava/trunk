/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

//#define _GLDEBUG_IN_RELEASE

#include "config.h"

#include "Platform/Platform/Headers/PlatformDefines.h"
#ifndef GLBINDING_STATIC
#define GLBINDING_STATIC
#endif

#define GL_VERSION_4_4
//#define GL_VERSION_4_5

#ifdef GL_VERSION_4_5
#include <glbinding/gl45/gl.h>
using namespace gl45;
namespace glext = gl45ext;
#else
#include <glbinding/gl44/gl.h>
using namespace gl44;
namespace glext = gl44ext;
#endif

#include <glbinding/Binding.h>

#include "Platform/Video/Headers/RenderAPIWrapper.h"

struct SDL_Window;
typedef void *SDL_GLContext;

namespace NS_GLIM {
    enum class GLIM_ENUM : int;
}; //namespace NS_GLIM

namespace Divide {
namespace GLUtil {

/// Wrapper for glGetIntegerv
GLint getIntegerv(GLenum param);
/// Check the current operation for errors
void APIENTRY 
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
              GLsizei length, const char* message, const void* userParam);
/// Invalid object value. Used to compare handles and determine if they were
/// properly created
extern GLuint _invalidObjectID;
/// Main rendering window
extern SDL_Window* _mainWindow;
extern SDL_GLContext _glRenderContext;
extern SDL_GLContext _glLoadingContext;
/// Populate enumeration tables with appropriate API values
void fillEnumTables();

extern std::array<GLenum, to_const_uint(BlendProperty::COUNT)> glBlendTable;
extern std::array<GLenum, to_const_uint(BlendOperation::COUNT)> glBlendOpTable;
extern std::array<GLenum, to_const_uint(ComparisonFunction::COUNT)>
    glCompareFuncTable;
extern std::array<GLenum, to_const_uint(StencilOperation::COUNT)>
    glStencilOpTable;
extern std::array<GLenum, to_const_uint(CullMode::COUNT)> glCullModeTable;
extern std::array<GLenum, to_const_uint(FillMode::COUNT)> glFillModeTable;
extern std::array<GLenum, to_const_uint(TextureType::COUNT)> glTextureTypeTable;
extern std::array<GLenum, to_const_uint(GFXImageFormat::COUNT)>
    glImageFormatTable;
extern std::array<GLenum, to_const_uint(PrimitiveType::COUNT)>
    glPrimitiveTypeTable;
extern std::array<GLenum, to_const_uint(GFXDataFormat::COUNT)> glDataFormat;
extern std::array<GLenum, to_const_uint(TextureWrap::COUNT)> glWrapTable;
extern std::array<GLenum, to_const_uint(TextureFilter::COUNT)>
    glTextureFilterTable;
extern std::array<NS_GLIM::GLIM_ENUM, to_const_uint(PrimitiveType::COUNT)>
    glimPrimitiveType;
extern std::array<GLenum, to_const_uint(ShaderType::COUNT)> glShaderStageTable;
extern std::array<stringImpl, to_const_uint(ShaderType::COUNT)>
    glShaderStageNameTable;

namespace DSAWrapper {
    const bool GL_USE_DSA = true;
#ifdef GL_VERSION_4_5
    const bool GL_USE_DSA_EXTENSION = false && GL_USE_DSA;
#endif

    // Textures
    void dsaCreateTextures(GLenum target, GLsizei count, GLuint* textures);
    void dsaGenerateTextureMipmap(GLuint texture, GLenum target);
    void dsaTextureStorage(GLuint texture, GLenum target, GLsizei levels,
                           GLenum internalformat, GLsizei width,
                           GLsizei height, GLsizei depth);
    void dsaTextureStorageMultisample(GLuint texture, GLenum target,
                                      GLsizei samples,
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height,
                                      GLsizei depth,
                                      GLboolean fixedsamplelocations);
    void dsaTextureSubImage(GLuint texture, GLenum target, GLint level,
                            GLint xoffset, GLint yoffset, GLint zoffset,
                            GLsizei width, GLsizei height, GLsizei depth,
                            GLenum format, GLenum type,
                            const bufferPtr pixels);

    void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname,
                                    GLfloat param);
    void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname,
                                    GLint param);

    // Samplers
    void dsaCreateSamplers(GLsizei count, GLuint* samplers);

    // VAO
    void dsaCreateVertexArrays(GLuint count, GLuint * arrays);
    // Buffers
    void dsaCreateBuffers(GLuint count, GLuint * buffers);
    GLboolean dsaUnmapNamedBuffer(GLuint buffer);
    void dsaNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const bufferPtr data);
    void dsaNamedBufferData(GLuint buffer, GLsizeiptr size, const bufferPtr data, GLenum usage);
    void dsaNamedBufferStorage(GLuint buffer, GLsizeiptr size, const bufferPtr data, BufferStorageMask flags);
    bufferPtr dsaMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, BufferAccessMask access);
    // Framebuffers
    void dsaCreateFramebuffers(GLuint count, GLuint * framebuffers);
    void dsaNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    void dsaNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
    void dsaNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf);
    void dsaNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum * bufs);
    void dsaNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src);
    GLenum dsaCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target);

};  // namespace DSAWrapper
};  // namespace GLUtil
};  // namespace Divide

#endif

#include "glResources.inl"