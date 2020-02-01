/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _GL_BUFFER_IMPL_H_
#define _GL_BUFFER_IMPL_H_

#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
namespace Divide {

struct BufferImplParams {
    GLenum _target = GL_NONE;

    /// Desired buffer size in bytes
    size_t _dataSize = 0;

    /// Buffer primitive size in bytes
    size_t _elementSize = 0;
    bool _zeroMem = false;
    bool _explicitFlush = true;
    bool _unsynced = true;
    const char* _name = nullptr;
    bufferPtr _initialData = NULL;
    BufferStorageType _storageType = BufferStorageType::AUTO;
    BufferUpdateFrequency _frequency = BufferUpdateFrequency::ONCE;
    BufferUpdateUsage _updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
};

struct BufferWriteData {
    BufferWriteData() noexcept : BufferWriteData(GLUtil::k_invalidObjectID) {};
    BufferWriteData(GLuint bufferHandle) noexcept : BufferWriteData(bufferHandle, 0, 0) {}
    BufferWriteData(GLuint bufferHandle, GLintptr offset, GLsizeiptr range) noexcept : BufferWriteData(bufferHandle, offset, range, false) {}
    BufferWriteData(GLuint bufferHandle, GLintptr offset, GLsizeiptr range, bool flush) noexcept : _handle(bufferHandle), _offset(offset), _range(range), _flush(flush) {}

    GLuint _handle = GLUtil::k_invalidObjectID;
    GLintptr _offset = 0;
    GLsizeiptr _range = 0;
    bool _flush = false;
};

class glBufferLockManager;
class glBufferImpl : public glObject, public GUIDWrapper {
public:
    explicit glBufferImpl(GFXDevice& context, const BufferImplParams& params);
    virtual ~glBufferImpl();

    GLuint bufferID() const noexcept;

    bool bindRange(GLuint bindIndex, GLintptr offsetInBytes, GLsizeiptr rangeInBytes);
    void lockRange(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, bool flush);
    bool waitRange(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, bool blockClient);

    void writeData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, bufferPtr data);
    void readData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, const bufferPtr data);
    void zeroMem(GLintptr offsetInBytes, GLsizeiptr rangeInBytes);

    size_t elementSize() const noexcept;

    static GLenum GetBufferUsage(BufferUpdateFrequency frequency, BufferUpdateUsage usage) noexcept;
protected:
    void invalidateData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes);

protected:
    GLenum _usage = GL_NONE;
    GLuint _handle = 0;
    bufferPtr _mappedBuffer = nullptr;
    GFXDevice& _context;
    const size_t _elementSize;
    const GLsizeiptr _alignedSize;
    const GLenum _target;
    const bool _unsynced;
    const bool _useExplicitFlush;
    const BufferUpdateFrequency _updateFrequency;
    const BufferUpdateUsage _updateUsage;
};
}; //namespace Divide

#endif //_GL_BUFFER_IMPL_H_