/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GL_UNIFORM_BUFFER_OBJECT_H_
#define GL_UNIFORM_BUFFER_OBJECT_H_

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

///Base class for shader uniform blocks
class glBufferLockManager;
class glUniformBuffer : public ShaderBuffer {
public:
    glUniformBuffer(bool unbound, bool persistentMapped);
    ~glUniformBuffer();

    ///Create a new buffer object to hold our uniform shader data
    virtual void Create(U32 primitiveCount, ptrdiff_t primitiveSize);
    virtual void DiscardAllData();
    virtual void DiscardSubData(ptrdiff_t offset, ptrdiff_t size);
    virtual void UpdateData(GLintptr offset, GLsizeiptr size, const void *data, const bool invalidateBuffer = false) const;
    virtual bool BindRange(ShaderBufferLocation bindIndex, U32 offsetElementCount, U32 rangeElementCount) const;
    virtual bool Bind(ShaderBufferLocation bindIndex) const;

    void PrintInfo(const ShaderProgram* shaderProgram, ShaderBufferLocation bindIndex);

protected:
    GLuint _UBOid;
    GLenum _target;
    void*  _mappedBuffer;
    glBufferLockManager* _lockManager;
};

}; //namespace Divide

#endif