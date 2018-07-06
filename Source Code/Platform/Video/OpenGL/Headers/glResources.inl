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

#ifndef _GL_RESOURCES_INL_
#define _GL_RESOURCES_INL_

namespace Divide {
namespace GLUtil {
namespace DSAWrapper {

// Textures
FORCE_INLINE void dsaCreateTextures(GLenum target, GLsizei count, GLuint* textures) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glCreateTextures(target, count, textures);
        return;
    }
#endif
    glGenTextures(count, textures);
}

FORCE_INLINE void dsaGenerateTextureMipmap(GLuint texture, GLenum target) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glGenerateTextureMipmap(texture);
        return;
    }
#endif
    glext::glGenerateTextureMipmapEXT(texture, target);
}

FORCE_INLINE void dsaTextureStorage(GLuint texture, GLenum target, GLsizei levels,
                                    GLenum internalformat, GLsizei width,
                                    GLsizei height, GLsizei depth) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        if (height == -1 && depth == -1) {
            glTextureStorage1D(texture, levels, internalformat, width);
        } else if (depth == -1) {
            glTextureStorage2D(texture, levels, internalformat, width, height);
        } else {
            glTextureStorage3D(texture, levels, internalformat, width, height, depth);
        }
        return;
    }
#endif
    if (height == -1 && depth == -1) {
        glext::glTextureStorage1DEXT(texture, target, levels, internalformat, width);
    } else if (depth == -1) {
        glext::glTextureStorage2DEXT(texture, target, levels, internalformat, width, height);
    } else {
        glext::glTextureStorage3DEXT(texture, target, levels, internalformat, width, height, depth);
    }
}

FORCE_INLINE void dsaTextureStorageMultisample(GLuint texture, GLenum target,
                                               GLsizei samples, GLenum internalformat,
                                               GLsizei width, GLsizei height,
                                               GLsizei depth, GLboolean fixedsamplelocations) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        if (depth == -1) {
            glTextureStorage2DMultisample(texture, samples, internalformat,
                                            width, height, fixedsamplelocations);
        } else {
            glTextureStorage3DMultisample(texture, samples, internalformat,
                                            width, height, depth, fixedsamplelocations);
        }
        return;
    }
#endif
    if (depth == -1) {
        glext::glTextureStorage2DMultisampleEXT(texture, target, samples,
                                                internalformat, width,
                                                height, fixedsamplelocations);
    } else {
        glext::glTextureStorage3DMultisampleEXT(texture, target, samples, 
                                                internalformat, width,
                                                height, depth, fixedsamplelocations);
    }
}

FORCE_INLINE void dsaTextureSubImage(GLuint texture, GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset, GLint zoffset,
                                     GLsizei width, GLsizei height, GLsizei depth,
                                     GLenum format, GLenum type, const bufferPtr pixels) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        if (height == -1 && depth == -1) {
            glTextureSubImage1D(texture, level, xoffset, width,
                                format, type, pixels);
        } else if (depth == -1) {
            glTextureSubImage2D(texture, level, xoffset, yoffset, width,
                                height, format, type, pixels);
        } else {
            glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset,
                                width, height, depth, format, type, pixels);
        }
        return;
    }
#endif

    if (height == -1 && depth == -1) {
        glext::glTextureSubImage1DEXT(texture, target, level, xoffset, width,
                                        format, type, pixels);
    } else if (depth == -1 || target == GL_TEXTURE_CUBE_MAP) {
        glext::glTextureSubImage2DEXT(texture, target == GL_TEXTURE_CUBE_MAP
                                                    ? GL_TEXTURE_CUBE_MAP_POSITIVE_X
                                                        + static_cast<GLuint>(zoffset)
                                                    : target,
                                        level, xoffset, yoffset, width, height, format,
                                        type, pixels);
    } else {
        glext::glTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset,
                                        width, height, depth, format, type, pixels);
    }
}

FORCE_INLINE void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname, GLfloat param) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glTextureParameterf(texture, pname, param);
        return;
    }
#endif
    glext::glTextureParameterfEXT(texture, target, pname, param);
}

FORCE_INLINE void dsaTextureParameter(GLuint texture, GLenum target, GLenum pname, GLint param) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glTextureParameteri(texture, pname, param);
        return;
    }
#endif
    glext::glTextureParameteriEXT(texture, target, pname, param);
}

// Samplers
FORCE_INLINE void dsaCreateSamplers(GLsizei count, GLuint* samplers) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glCreateSamplers(count, samplers);
        return;
    }
#endif

    glGenSamplers(count, samplers);
}

// VAO
FORCE_INLINE void dsaCreateVertexArrays(GLuint count, GLuint* arrays) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glCreateVertexArrays(count, arrays);
        return;
    }
#endif

    glGenVertexArrays(count, arrays);
}

// Buffers
FORCE_INLINE void dsaCreateBuffers(GLuint count, GLuint* buffers) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glCreateBuffers(count, buffers);
        return;
    }
#endif

    glGenBuffers(count, buffers);
}

FORCE_INLINE void dsaNamedBufferSubData(GLuint buffer, GLintptr offset,
                                        GLsizeiptr size, const bufferPtr data) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedBufferSubData(buffer, offset, size, data);
        return;
    }
#endif

    glext::glNamedBufferSubDataEXT(buffer, offset, size, data);
}

FORCE_INLINE void dsaNamedBufferData(GLuint buffer, GLsizeiptr size,
                                     const bufferPtr data, GLenum usage) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedBufferData(buffer, size, data, usage);
        return;
    }
#endif

    glext::glNamedBufferDataEXT(buffer, size, data, usage);
}

FORCE_INLINE GLboolean dsaUnmapNamedBuffer(GLuint buffer) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        return glUnmapNamedBuffer(buffer);
    }
#endif

    return glext::glUnmapNamedBufferEXT(buffer);
}

FORCE_INLINE void dsaNamedBufferStorage(GLuint buffer, GLsizeiptr size,
                                        const bufferPtr data, BufferStorageMask flags) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedBufferStorage(buffer, size, data, flags);
        return;
    }
#endif

    glext::glNamedBufferStorageEXT(buffer, size, data, flags);
}

FORCE_INLINE bufferPtr dsaMapNamedBufferRange(GLuint buffer, GLintptr offset,
                                              GLsizeiptr length, BufferAccessMask access) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        return glMapNamedBufferRange(buffer, offset, length, access);
    }
#endif

    return glext::glMapNamedBufferRangeEXT(buffer, offset, length, access);
}

// Framebuffers
FORCE_INLINE void dsaCreateFramebuffers(GLuint count, GLuint* framebuffers) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glCreateFramebuffers(count, framebuffers);
        return;
    }
#endif

    glGenFramebuffers(count, framebuffers);
}

FORCE_INLINE void dsaNamedFramebufferTexture(GLuint framebuffer, GLenum attachment,
                                             GLuint texture, GLint level) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedFramebufferTexture(framebuffer, attachment, texture, level);
        return;
    }
#endif
    glext::glNamedFramebufferTextureEXT(framebuffer, attachment, texture, level);
}

FORCE_INLINE void dsaNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment,
                                                  GLuint texture, GLint level, GLint layer) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
        return;
    }
#endif

    glext::glNamedFramebufferTextureLayerEXT(framebuffer, attachment, texture, level, layer);
}

FORCE_INLINE void dsaNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedFramebufferDrawBuffer(framebuffer, buf);
        return;
    }
#endif
    glext::glFramebufferDrawBufferEXT(framebuffer, buf);
}


FORCE_INLINE void dsaNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs) {
#ifdef GL_VERSION_4_5
if (!GL_USE_DSA_EXTENSION) {
    glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
    return;
}
#endif
glext::glFramebufferDrawBuffersEXT(framebuffer, n, bufs);
}

FORCE_INLINE void dsaNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        glNamedFramebufferReadBuffer(framebuffer, src);
        return;
    }
#endif
    glext::glFramebufferReadBufferEXT(framebuffer, src);
}

FORCE_INLINE GLenum dsaCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
#ifdef GL_VERSION_4_5
    if (!GL_USE_DSA_EXTENSION) {
        return glCheckNamedFramebufferStatus(framebuffer, target);
    }
#endif

    return glext::glCheckNamedFramebufferStatusEXT(framebuffer, target);
}

}; // namespace DSAWrapper
};  // namespace GLUtil
}; // namespace Divide
#endif  //_GL_RESOURCES_INL_