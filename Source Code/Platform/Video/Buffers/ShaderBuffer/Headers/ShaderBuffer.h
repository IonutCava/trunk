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

#ifndef _SHADER_BUFFER_H_
#define _SHADER_BUFFER_H_

#include "Core/Headers/RingBuffer.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {

struct ShaderBufferDescriptor {
    ShaderBufferDescriptor()
        : _ringBufferLength(1),
          _primitiveCount(0),
          _primitiveSizeInBytes(0),
          _unbound(false),
          _initialData(nullptr),
          _updateFrequency(BufferUpdateFrequency::ONCE)
    {
    }

    U32 _ringBufferLength;
    U32 _primitiveCount;
    size_t _primitiveSizeInBytes;
    bool _unbound;
    BufferUpdateFrequency _updateFrequency;
    bufferPtr _initialData;
};

class ShaderProgram;
class NOINITVTABLE ShaderBuffer : public GUIDWrapper,
                                  public GraphicsResource,
                                  public RingBuffer
{
    USE_CUSTOM_ALLOCATOR
   public:
    explicit ShaderBuffer(GFXDevice& context, const ShaderBufferDescriptor& params);

    virtual ~ShaderBuffer();

    virtual void writeData(ptrdiff_t offsetElementCount,
                           ptrdiff_t rangeElementCount,
                           const bufferPtr data) = 0;

    virtual void writeData(const bufferPtr data);

    virtual void readData(ptrdiff_t offsetElementCount,
                          ptrdiff_t rangeElementCount,
                          bufferPtr result) const = 0;

    virtual bool bindRange(U32 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    /// Bind return false if the buffer was already bound
    virtual bool bind(U32 bindIndex) = 0;

    inline bool bind(ShaderBufferLocation bindIndex) {
        return bind(to_U32(bindIndex));
    }

    inline bool bindRange(ShaderBufferLocation bindIndex,
                          U32 offsetElementCount,
                          U32 rangeElementCount) {
        return bindRange(to_U32(bindIndex),
                         offsetElementCount,
                         rangeElementCount);

    }

    inline size_t getPrimitiveSize() const { return _primitiveSize; }
    inline U32 getPrimitiveCount() const { return _primitiveCount; }

    virtual void addAtomicCounter(U32 sizeFactor = 1) = 0;
    virtual U32  getAtomicCounter(U32 counterIndex = 0) = 0;
    virtual void bindAtomicCounter(U32 counterIndex = 0, U32 bindIndex = 0) = 0;
    virtual void resetAtomicCounter(U32 counterIndex = 0) = 0;

    static size_t alignmentRequirement(bool unbound);

   protected:
    size_t _bufferSize;
    size_t _primitiveSize;
    size_t _maxSize;
    U32 _primitiveCount;

    static size_t _boundAlignmentRequirement;
    static size_t _unboundAlignmentRequirement;

    const bool _unbound;
    const BufferUpdateFrequency _frequency;
};


};  // namespace Divide
#endif //_SHADER_BUFFER_H_
