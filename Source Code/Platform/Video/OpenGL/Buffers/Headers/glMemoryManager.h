/*
Copyright (c) 2015 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _GL_MEMORY_MANAGER_H_
#define _GL_MEMORY_MANAGER_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"

namespace Divide {
namespace GLUtil {
    void allocBuffer(GLuint bufferId,
                     GLsizeiptr bufferSize,
                     GLenum usageMask,
                     const bufferPtr data = NULL);

    void createAndAllocBuffer(GLsizeiptr bufferSize,
                              GLenum usageMask,
                              GLuint& bufferIdOut,
                              const bufferPtr data = NULL);

    bufferPtr allocPersistentBuffer(GLuint bufferId,
                                    GLsizeiptr bufferSize,
                                    MapBufferUsageMask usageMask,
                                    BufferAccessMask accessMask,
                                    const bufferPtr data = NULL);

    bufferPtr createAndAllocPersistentBuffer(GLsizeiptr bufferSize,
                                             MapBufferUsageMask usageMask,
                                             BufferAccessMask accessMask,
                                             GLuint& bufferIdOut,
                                             const bufferPtr data = NULL);

    void updateBuffer(GLuint bufferId,
                      GLintptr offset,
                      GLsizeiptr size,
                      const bufferPtr data);

    void freeBuffer(GLuint &bufferId, bufferPtr mappedPtr = nullptr);

}; //namespace GLUtil
}; //namespace Divide

#endif //_GL_MEMORY_MANAGER_H_