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
    bool _explicitFlush = true;
    bool _unsynced = true;
    const char* _name = nullptr;
    bufferPtr _initialData = NULL;
    BufferStorageType _storageType = BufferStorageType::AUTO;
    BufferUpdateFrequency _frequency = BufferUpdateFrequency::ONCE;
    BufferUpdateUsage _updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
};

class glBufferImpl;
struct BufferLockEntry {
    glBufferImpl* _buffer = nullptr;
    size_t _offset = 0;
    size_t _length = 0;
    bool   _flush = false;
};

class glBufferLockManager;
class glBufferImpl : public glObject, public GUIDWrapper {
public:
    explicit glBufferImpl(GFXDevice& context, const BufferImplParams& params);
    virtual ~glBufferImpl();

    GLuint bufferID() const noexcept;

    bool bindRange(GLuint bindIndex, size_t offsetInBytes, size_t rangeInBytes);
    void lockRange(size_t offsetInBytes, size_t rangeInBytes, U32 frameID);
    bool waitRange(size_t offsetInBytes, size_t rangeInBytes, bool blockClient);

    void writeData(size_t offsetInBytes, size_t rangeInBytes, const Byte* data);
    void readData(size_t offsetInBytes, size_t rangeInBytes, Byte* data);
    void zeroMem(size_t offsetInBytes, size_t rangeInBytes);

    size_t elementSize() const noexcept;

    static GLenum GetBufferUsage(BufferUpdateFrequency frequency, BufferUpdateUsage usage) noexcept;

protected:
    void invalidateData(size_t offsetInBytes, size_t rangeInBytes);

protected:
    GLenum _usage = GL_NONE;
    GLuint _handle = 0;
    Byte*  _mappedBuffer = nullptr;
    GFXDevice& _context;
    const size_t _elementSize;
    const size_t _alignedSize;
    const GLenum _target;
    const bool _unsynced;
    const bool _useExplicitFlush;
    const BufferUpdateFrequency _updateFrequency;
    const BufferUpdateUsage _updateUsage;

    glBufferLockManager* _lockManager = nullptr;
};
}; //namespace Divide

#endif //_GL_BUFFER_IMPL_H_