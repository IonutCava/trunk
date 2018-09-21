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
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {

struct ShaderBufferDescriptor {
    U32 _flags = 0;
    U32 _ringBufferLength = 1;
    U32 _elementCount = 0;
    size_t _elementSize = 0; //< Primitive size in bytes
    BufferUpdateFrequency _updateFrequency = BufferUpdateFrequency::ONCE;
    bufferPtr _initialData = NULL;
    stringImpl _name = "";
};

class ShaderProgram;
class NOINITVTABLE ShaderBuffer : public GUIDWrapper,
                                  public GraphicsResource,
                                  public RingBuffer
{
    USE_CUSTOM_ALLOCATOR
   public:
       enum class Flags : U8 {
           UNBOUND_STORAGE = toBit(1),
           ALLOW_THREADED_WRITES = toBit(2),
           AUTO_RANGE_FLUSH = toBit(3),
           COUNT = 2
       };

   public:
    explicit ShaderBuffer(GFXDevice& context, const ShaderBufferDescriptor& params);

    virtual ~ShaderBuffer();

    virtual void writeData(ptrdiff_t offsetElementCount,
                           ptrdiff_t rangeElementCount,
                           const bufferPtr data) = 0;

    virtual void writeBytes(ptrdiff_t offsetInBytes,
                            ptrdiff_t rangeInBytes,
                            const bufferPtr data) = 0;

    virtual void writeData(const bufferPtr data);

    virtual void readData(ptrdiff_t offsetElementCount,
                          ptrdiff_t rangeElementCount,
                          bufferPtr result) const = 0;

    virtual bool bindRange(U8 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount,
                           size_t& offsetOut,
                           size_t& rangeOut) = 0;

    /// Bind return false if the buffer was already bound
    virtual bool bind(U8 bindIndex,
                      size_t& offsetOut,
                      size_t& rangeOut) = 0;
    inline bool bind(ShaderBufferLocation bindIndex) {
        size_t offsetOut = 0, rangeOut = 0;
        return bind(bindIndex, offsetOut, rangeOut);
    }
    inline bool bind(ShaderBufferLocation bindIndex,
                     size_t& offsetOut,
                     size_t& rangeOut) {
        return bind(to_U8(bindIndex), offsetOut, rangeOut);
    }

    inline bool bindRange(ShaderBufferLocation bindIndex,
                          U32 offsetElementCount,
                          U32 rangeElementCount,
                           size_t& offsetOut,
                           size_t& rangeOut) {
        return bindRange(to_U8(bindIndex),
                         offsetElementCount,
                         rangeElementCount,
                         offsetOut,
                         rangeOut);
    }

    inline U32 getPrimitiveCount() const { return _elementCount; }

    virtual void addAtomicCounter(U32 sizeFactor, U16 ringSizeFactor = 1) = 0;
    virtual U32  getAtomicCounter(U8 offset, U8 counterIndex = 0) = 0;
    virtual void bindAtomicCounter(U8 offset, U8 counterIndex = 0, U8 bindIndex = 0) = 0;
    virtual void resetAtomicCounter(U8 offset, U8 counterIndex = 0) = 0;

    static size_t alignmentRequirement(bool unbound);

   protected:
    size_t _bufferSize;
    size_t _maxSize;
    U32 _elementCount;

    static size_t _boundAlignmentRequirement;
    static size_t _unboundAlignmentRequirement;

    const U32 _flags;
    const BufferUpdateFrequency _frequency;

    stringImpl _name;
};

};  // namespace Divide
#endif //_SHADER_BUFFER_H_
