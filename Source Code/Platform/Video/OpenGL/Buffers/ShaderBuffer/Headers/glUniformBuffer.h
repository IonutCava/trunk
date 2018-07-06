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

#ifndef GL_UNIFORM_BUFFER_OBJECT_H_
#define GL_UNIFORM_BUFFER_OBJECT_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

/// Base class for shader uniform blocks
class glBufferLockManager;
class glUniformBuffer final : public ShaderBuffer {
   public:
     glUniformBuffer(GFXDevice& context,
                     const stringImpl &bufferName,
                     const U32 ringBufferLength,
                     bool unbound,
                     bool persistentMapped,
                     BufferUpdateFrequency frequency);
    ~glUniformBuffer();

    void destroy() override;
    /// Create a new buffer object to hold our uniform shader data
    void create(U32 primitiveCount, ptrdiff_t primitiveSize, U32 sizeFactor = 1) override;

    void getData(GLintptr offsetElementCount,
                 GLsizeiptr rangeElementCount,
                 bufferPtr result,
                 U32 sizeFactorOffset = 0) const override;

    void updateData(GLintptr offsetElementCount,
                    GLsizeiptr rangeElementCount,
                    const bufferPtr data,
                    U32 sizeFactorOffset = 0) override;

    bool bindRange(U32 bindIndex,
                   U32 offsetElementCount,
                   U32 rangeElementCount,
                   U32 sizeFactorOffset = 0) override;

    bool bind(U32 bindIndex, U32 sizeFactorOffset = 0) override;

    GLuint getBufferID() const { return _UBOid; }

    void addAtomicCounter(U32 sizeFactor = 1) override;

    U32  getAtomicCounter(U32 counterIndex = 0) override;

    void bindAtomicCounter(U32 counterIndex = 0, U32 bindIndex = 0) override;

    void resetAtomicCounter(U32 counterIndex = 0) override;

   protected:
    void printInfo(const ShaderProgram *shaderProgram, U32 bindIndex);

   protected:

    struct AtomicCounter {
        GLuint _handle;
        GLuint _sizeFactor;
        GLuint _writeHead;
        GLuint _readHead;
    };

    GLuint _UBOid;
    size_t     _allignedBufferSize;
    GLsizeiptr _alignment;
    bufferPtr _mappedBuffer;
    bool _updated;
    const GLenum _target;
    const std::unique_ptr<glBufferLockManager> _lockManager;
    vectorImpl<AtomicCounter> _atomicCounters;
};

};  // namespace Divide

#endif