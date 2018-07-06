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

#ifndef _SHADER_BUFFER_H_
#define _SHADER_BUFFER_H_

#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {

class ShaderProgram;
class NOINITVTABLE ShaderBuffer : protected GraphicsResource, 
                                  public RingBuffer,
                                  public GUIDWrapper {
   public:
    explicit ShaderBuffer(GFXDevice& context, 
                          const stringImpl& bufferName,
                          const U32 ringBufferLength,
                          bool unbound,
                          bool persistentMapped,
                          BufferUpdateFrequency frequency);

    virtual ~ShaderBuffer();

    /// Create a new buffer to hold our shader data
    virtual void create(U32 primitiveCount, ptrdiff_t primitiveSize, U32 sizeFactor = 1);
    virtual void destroy() = 0;

    virtual void updateData(ptrdiff_t offsetElementCount,
                            ptrdiff_t rangeElementCount,
                            const bufferPtr data) = 0;

    virtual void setData(const bufferPtr data);

    virtual void getData(ptrdiff_t offsetElementCount,
                         ptrdiff_t rangeElementCount,
                         bufferPtr result) const = 0;

    virtual bool bindRange(U32 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    virtual bool bind(U32 bindIndex) = 0;

    inline bool bind(ShaderBufferLocation bindIndex) {
        return bind(to_uint(bindIndex));
    }

    inline bool bindRange(ShaderBufferLocation bindIndex,
                          U32 offsetElementCount,
                          U32 rangeElementCount) {
        return bindRange(to_uint(bindIndex),
                         offsetElementCount,
                         rangeElementCount);

    }

    inline size_t getPrimitiveSize() const { return _primitiveSize; }
    inline U32 getPrimitiveCount() const { return _primitiveCount; }
    inline size_t getAlignmentRequirement() const { return _alignmentRequirement; }

    virtual void addAtomicCounter(U32 sizeFactor = 1) = 0;
    virtual U32  getAtomicCounter(U32 counterIndex = 0) = 0;
    virtual void bindAtomicCounter(U32 counterIndex = 0, U32 bindIndex = 0) = 0;
    virtual void resetAtomicCounter(U32 counterIndex = 0) = 0;

   protected:
    size_t _bufferSize;
    size_t _primitiveSize;
    size_t _alignmentRequirement;
    U32 _primitiveCount;
    U32 _sizeFactor;

    const bool _unbound;
    const bool _persistentMapped;
    const BufferUpdateFrequency _frequency;

#   if defined(ENABLE_GPU_VALIDATION)
    stringImpl _bufferName;
#   endif
};

};  // namespace Divide
#endif //_SHADER_BUFFER_H_
