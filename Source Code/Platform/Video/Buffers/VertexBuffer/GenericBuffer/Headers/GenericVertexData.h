/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _GENERIC_VERTEX_DATA_H
#define _GENERIC_VERTEX_DATA_H

#include "config.h"

#include "AttributeDescriptor.h"
#include "Core/Headers/RingBuffer.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

/// This class is used to upload generic VB data to the GPU that can be rendered directly or instanced.
/// Use this class to create precise VB data with specific usage (such as particle systems)
/// Use IMPrimitive for on-the-fly geometry creation

namespace Divide {

class NOINITVTABLE GenericVertexData : public VertexDataInterface,
                                       public RingBuffer {
   public:
     
     struct IndexBuffer {
         size_t count = 0;
         size_t offsetCount = 0;
         bufferPtr data = nullptr;
         bool smallIndices = false;
     };

     struct SetBufferParams {
         U32 _buffer = 0;
         U32 _elementCount = 0;
         U32 _instanceDivisor = 0;
         size_t _elementSize = 0;
         bool _sync = false;
         bool _useRingBuffer = false;
         bufferPtr _data = nullptr;
         BufferStorageType _storageType = BufferStorageType::AUTO;
         BufferUpdateFrequency _updateFrequency = BufferUpdateFrequency::COUNT;
         BufferUpdateUsage _updateUsage = BufferUpdateUsage::COUNT;
     };

   public:
    GenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name = nullptr);
    virtual ~GenericVertexData();

    virtual void setIndexBuffer(const IndexBuffer& indices, BufferUpdateFrequency updateFrequency);
    virtual void updateIndexBuffer(const IndexBuffer& indices);

    virtual void create(U8 numBuffers = 1) = 0;

    virtual void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) = 0;

    /// When reading and writing to the same buffer, we use a round-robin approach and
    /// offset the reading and writing to multiple copies of the data
    virtual void setBuffer(const SetBufferParams& params) = 0;

    virtual void updateBuffer(U32 buffer,
                              U32 elementCount,
                              U32 elementCountOffset,
                              const bufferPtr data) = 0;

    virtual void setBufferBindOffset(U32 buffer, U32 elementCountOffset) = 0;
    
    AttributeDescriptor& attribDescriptor(U32 attribIndex);

    inline const IndexBuffer& indexBuffer() const { return _idxBuffer; }

   protected:
    using AttributeMap = hashMap<U32, AttributeDescriptor>;
    AttributeMap _attributeMapDraw;

    stringImpl _name;
    IndexBuffer _idxBuffer;
};

};  // namespace Divide

#endif
