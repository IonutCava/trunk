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

#ifndef _GENERIC_VERTEX_DATA_H
#define _GENERIC_VERTEX_DATA_H
#include <boost/noncopyable.hpp>
#include "Utility/Headers/Vector.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
/// This class is used to upload generic VB data to the GPU that can be rendered directly or instanced.
/// Use this class to create precise VB data with specific usage (such as particle systems)
/// Use IMPrimitive for on-the-fly geometry creation

class GenericVertexData : private boost::noncopyable {
public:
    GenericVertexData() : _numBuffers(0)
    {
    }

    virtual ~GenericVertexData()
    {
    }

    virtual void Clear() = 0;
    virtual void Create(U8 numBuffers = 1) = 0;
    virtual void Draw(const PrimitiveType& type, U32 min, U32 max) = 0;
    virtual void DrawInstanced(const PrimitiveType& type, U32 count, U32 min, U32 max) = 0;

    virtual void SetBuffer(U32 buffer, size_t dataSize, void* data, bool dynamic, bool stream) = 0;
    virtual void UpdateBuffer(U32 buffer, size_t dataSize, void* data, U32 offset, size_t currentSize, bool dynamic, bool steam) = 0;
    virtual void SetAttribute(U32 index, U32 buffer, U32 divisor, size_t size, bool normalized, U32 stride, U32 offset, const GFXDataFormat& type) = 0;

protected:
    U8 _numBuffers;
    vectorImpl<U32 > _bufferObjects;
};
#endif
