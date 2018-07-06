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

#ifndef D3D_UNIFORM_BUFFER_OBJECT_H_
#define D3D_UNIFORM_BUFFER_OBJECT_H_

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
namespace Divide {

///Base class for shader constant buffers
class d3dConstantBuffer : public ShaderBuffer {
public:
    d3dConstantBuffer(bool unbound, bool persistentMapped);
    ~d3dConstantBuffer();
    
    ///Reserve primitiveCount * implementation specific primitive size of space in the buffer and fill it with NULL values
    virtual void Create(U32 primitiveCount, ptrdiff_t primitiveSize);
    virtual void DiscardAllData();
    virtual void DiscardSubData(ptrdiff_t offset, ptrdiff_t size);
    virtual void UpdateData(ptrdiff_t offset, ptrdiff_t size, const void *data, const bool invalidateBuffer = false) const;
    virtual bool BindRange(ShaderBufferLocation bindIndex, U32 offsetElementCount, U32 rangeElementCount) const;
    virtual bool Bind(ShaderBufferLocation bindIndex) const;
    virtual void PrintInfo(const ShaderProgram* shaderProgram, ShaderBufferLocation bindIndex);
};

}; //namespace Divide
#endif