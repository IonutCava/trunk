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

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

class ShaderProgram;
class NOINITVTABLE ShaderBuffer : private NonCopyable, public RingBuffer, public GUIDWrapper {
   public:
    explicit ShaderBuffer(const stringImpl& bufferName, const U32 sizeFactor, bool unbound, bool persistentMapped);

    virtual ~ShaderBuffer();

    /// Create a new buffer to hold our shader data
    virtual void Create(U32 primitiveCount, ptrdiff_t primitiveSize);
    virtual void Destroy() = 0;

    virtual void UpdateData(ptrdiff_t offsetElementCount,
                            ptrdiff_t rangeElementCount,
                            const bufferPtr data) const = 0;

    virtual void SetData(const bufferPtr data);

    virtual bool BindRange(U32 bindIndex,
                           U32 offsetElementCount,
                           U32 rangeElementCount) = 0;

    virtual bool Bind(U32 bindIndex) = 0;

    inline bool Bind(ShaderBufferLocation bindIndex) {
        return Bind(to_uint(bindIndex));
    }

    inline bool BindRange(ShaderBufferLocation bindIndex,
                          U32 offsetElementCount,
                          U32 rangeElementCount) {
        return BindRange(to_uint(bindIndex),
                         offsetElementCount,
                         rangeElementCount);

    }

    inline size_t getPrimitiveSize() const { return _primitiveSize; }
    inline U32 getPrimitiveCount() const { return _primitiveCount; }

   protected:
    size_t _bufferSize;
    size_t _primitiveSize;
    U32 _primitiveCount;

    const bool _unbound;
    const bool _persistentMapped;
};

};  // namespace Divide
#endif //_SHADER_BUFFER_H_
