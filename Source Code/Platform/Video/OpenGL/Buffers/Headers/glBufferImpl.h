/*
Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _GL_BUFFER_IMPL_H_
#define _GL_BUFFER_IMPL_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"
namespace Divide {

class glBufferLockManager;
class glBufferImpl {
public:
    glBufferImpl(GLenum target);
    virtual ~glBufferImpl();

    GLuint bufferID() const;

    virtual void create(BufferUpdateFrequency frequency, size_t size);
    virtual bool bindRange(GLuint bindIndex, size_t offset, size_t range);
    virtual void lockRange(size_t offset, size_t range);
    virtual void destroy() = 0;
    virtual void updateData(size_t offset, size_t range, const bufferPtr data) = 0;
protected:
    GLenum _target;
    GLuint _handle;
    size_t _alignedSize;
};

class glRegularBuffer : public glBufferImpl {
public:
    glRegularBuffer(GLenum target);
    ~glRegularBuffer();

    void create(BufferUpdateFrequency frequency, size_t size) override;
    void destroy() override;
    void updateData(size_t offset, size_t range, const bufferPtr data) override;
};

class glPersistentBuffer : public glBufferImpl {
public:
    glPersistentBuffer(GLenum target);
    ~glPersistentBuffer();

    void create(BufferUpdateFrequency frequency, size_t size) override;
    void destroy() override;
    void updateData(size_t offset, size_t range, const bufferPtr data) override;
    bool bindRange(GLuint bindIndex, size_t offset, size_t range) override;
    void lockRange(size_t offset, size_t range) override;

private:
    bufferPtr _mappedBuffer;
    glBufferLockManager* _lockManager;
};
}; //namespace Divide

#endif //_GL_BUFFER_IMPL_H_