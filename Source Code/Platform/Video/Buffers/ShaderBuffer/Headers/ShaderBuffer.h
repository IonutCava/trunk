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
#ifndef _SHADER_BUFFER_H_
#define _SHADER_BUFFER_H_

#include "Core/Headers/RingBuffer.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

struct ShaderBufferDescriptor;

class ShaderProgram;
class NOINITVTABLE ShaderBuffer : public GUIDWrapper,
                                  public GraphicsResource,
                                  public RingBufferSeparateWrite
{
   public:
       enum class Usage : U8 {
           CONSTANT_BUFFER = 0,
           UNBOUND_BUFFER,
           ATOMIC_COUNTER,
           COUNT
       };

       enum class Flags : U8 {
           NONE = 0,
           IMMUTABLE_STORAGE = toBit(1),                         ///< Persistent mapped buffers
           ALLOW_THREADED_WRITES = toBit(2) | IMMUTABLE_STORAGE, ///< Makes sure reads and writes are properly synced between threads (e.g. With glFlush() after glFenceSync() in OpenGL). Will force IMMUTABLE_STORAGE to on
           AUTO_STORAGE = toBit(3),                              ///< Overrides Immutable flag
           AUTO_RANGE_FLUSH = toBit(4),                          ///< Flush the entire buffer after a write as opposed to just the affected region
           NO_SYNC = toBit(5),                                   ///< Skip any kind of sync between reads and writes. Useful if using stuff that auto-syncs like glBufferSubData in OpenGL
           COUNT = 6
       };

   public:
    explicit ShaderBuffer(GFXDevice& context, const ShaderBufferDescriptor& descriptor);

    virtual ~ShaderBuffer() = default;

    virtual void clearData(U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    virtual void writeData(U32 offsetElementCount,
                           U32 rangeElementCount,
                           bufferPtr data) = 0;

    virtual void writeBytes(ptrdiff_t offsetInBytes,
                            ptrdiff_t rangeInBytes,
                            bufferPtr data) = 0;

    virtual void writeData(bufferPtr data);

    virtual void readData(U32 offsetElementCount,
                          U32 rangeElementCount,
                          bufferPtr result) const = 0;

    virtual bool bindRange(U8 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    /// Bind return false if the buffer was already bound
    virtual bool bind(U8 bindIndex) = 0;

    bool bind(const ShaderBufferLocation bindIndex) {
        return bind(to_U8(bindIndex));
    }

    bool bindRange(const ShaderBufferLocation bindIndex,
                   const U32 offsetElementCount,
                   const U32 rangeElementCount) {
        return bindRange(to_U8(bindIndex),
                         offsetElementCount,
                         rangeElementCount);
    }

    U32 getPrimitiveCount() const noexcept { return _elementCount; }
    size_t getPrimitiveSize() const noexcept { return _elementSize; }

    static size_t alignmentRequirement(Usage usage);

   protected:
    size_t _bufferSize = 0;
    size_t _maxSize = 0;
    size_t _elementSize;
    U32 _elementCount;

    static size_t s_boundAlignmentRequirement;
    static size_t s_unboundAlignmentRequirement;

    const U32 _flags;
    const Usage _usage;
    const BufferUpdateFrequency _frequency;
    const BufferUpdateUsage _updateUsage;

    stringImpl _name;
};

struct ShaderBufferDescriptor {
    stringImpl _name = "";
    size_t _elementSize = 0; ///< Primitive size in bytes
    bufferPtr _initialData = nullptr;
    U32 _flags = 0u;
    U32 _ringBufferLength = 1u;
    U32 _elementCount = 0u;
    BufferUpdateFrequency _updateFrequency = BufferUpdateFrequency::ONCE;
    BufferUpdateUsage _updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    ShaderBuffer::Usage _usage = ShaderBuffer::Usage::COUNT;
    bool _separateReadWrite = false; ///< Use a separate read/write index based on queue length
};
};  // namespace Divide
#endif //_SHADER_BUFFER_H_
