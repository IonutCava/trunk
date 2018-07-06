/*
Copyright (c) 2017 DIVIDE-Studio
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

struct BufferImplParams {
    BufferImplParams()
        : _target(GL_NONE),
          _frequency(BufferUpdateFrequency::ONCE),
          _dataSizeInBytes(0),
          _initialData(NULL),
          _name("")
    {
    }

    GLenum _target;
    BufferUpdateFrequency _frequency;
    size_t _dataSizeInBytes;
    bufferPtr _initialData;
    const char* _name;
};

class glBufferLockManager;
class glBufferImpl : public glObject {
public:
    explicit glBufferImpl(GFXDevice& context, const BufferImplParams& params);
    virtual ~glBufferImpl();

    GLuint bufferID() const;

    bool bindRange(GLuint bindIndex, size_t offset, size_t range);
    void lockRange(size_t offset, size_t range);
    void waitRange(size_t offset, size_t range, bool blockClient);

    void writeData(size_t offset, size_t range, const bufferPtr data);
    void readData(size_t offset, size_t range, const bufferPtr data);

protected:
    GLenum _target;
    GLuint _handle;
    size_t _alignedSize;
    GLenum _usage;
    bufferPtr _mappedBuffer;
    BufferUpdateFrequency _updateFrequency;
    glBufferLockManager* _lockManager;
};
}; //namespace Divide

#endif //_GL_BUFFER_IMPL_H_