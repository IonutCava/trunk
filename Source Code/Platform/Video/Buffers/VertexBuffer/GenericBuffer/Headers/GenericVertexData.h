/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _GENERIC_VERTEX_DATA_H
#define _GENERIC_VERTEX_DATA_H

#include "config.h"

#include "AttributeDescriptor.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"

/// This class is used to upload generic VB data to the GPU that can be rendered directly or instanced.
/// Use this class to create precise VB data with specific usage (such as particle systems)
/// Use IMPrimitive for on-the-fly geometry creation

namespace Divide {

class NOINITVTABLE GenericVertexData : public VertexDataInterface,
                                       public RingBuffer {
   public:
    GenericVertexData(GFXDevice& context, const U32 ringBufferLength);
    virtual ~GenericVertexData();

    inline void setIndexBuffer(U32 indicesCount, bool dynamic, bool stream) {
        vectorImpl<U32> indices;
        setIndexBuffer(indicesCount, dynamic, stream);
    }

    virtual void setIndexBuffer(U32 indicesCount, bool dynamic, bool stream, const vectorImpl<U32>& indices) = 0;
    virtual void updateIndexBuffer(const vectorImpl<U32>& indices) = 0;
    virtual void create(U8 numBuffers = 1, U8 numQueries = 1) = 0;
    virtual void setFeedbackBuffer(U32 buffer, U32 bindPoint) = 0;

    virtual void draw(const GenericDrawCommand& command, bool useCmdBuffer = false) = 0;

    /// When reading and writing to the same buffer, we use a round-robin
    /// approach and
    /// offset the reading and writing to multiple copies of the data
    virtual void setBuffer(U32 buffer, U32 elementCount, size_t elementSize,
                           bool useRingBuffer, bufferPtr data, bool dynamic, bool stream,
                           bool persistentMapped = false) = 0;

    virtual void updateBuffer(U32 buffer, U32 elementCount,
                              U32 elementCountOffset, bufferPtr data) = 0;

    virtual void bindFeedbackBufferRange(U32 buffer, U32 elementCountOffset,
                                         size_t elementCount) = 0;

    virtual U32 getFeedbackPrimitiveCount(U8 queryID) = 0;

    virtual void incQueryQueue() = 0;

    void toggleDoubleBufferedQueries(const bool state);
    AttributeDescriptor& attribDescriptor(U32 attribIndex);
    AttributeDescriptor& fdbkAttribDescriptor(U32 attribIndex);

   protected:
    typedef hashMapImpl<U32, AttributeDescriptor> attributeMap;
    bool _doubleBufferedQuery;
    vectorImpl<std::pair<U32 /*buffer*/, U32/*bind point*/>> _feedbackBuffers;
    attributeMap _attributeMapDraw;
    attributeMap _attributeMapFdbk;
};

};  // namespace Divide

#endif
