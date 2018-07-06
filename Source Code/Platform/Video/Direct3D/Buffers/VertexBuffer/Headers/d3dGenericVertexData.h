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

#ifndef _D3D_GENERIC_VERTEX_DATA_H
#define _D3d_GENERIC_VERTEX_DATA_H

#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {

class d3dGenericVertexData : public GenericVertexData {
    USE_CUSTOM_ALLOCATOR
   public:
    d3dGenericVertexData(GFXDevice& context, const U32 ringBufferLength);

    ~d3dGenericVertexData();

    void destroy() override {}

    void create(U8 numBuffers = 1, U8 numQueries = 1) override {}

    void setIndexBuffer(U32 indicesCount, bool dynamic, bool stream, const vectorImpl<U32>& indices) override {}
    void updateIndexBuffer(const vectorImpl<U32>& indices) override {}

    void setBuffer(U32 buffer,
                   U32 elementCount,
                   size_t elementSize,
                   bool useRingBuffer,
                   const bufferPtr data,
                   bool dynamic,
                   bool stream,
                   bool persistentMapped = false) override {}

    void bindFeedbackBufferRange(U32 buffer,
                                 U32 elementCountOffset,
                                 size_t elementCount) override {}

    void updateBuffer(U32 buffer,
                      U32 elementCount,
                      U32 elementCountOffset,
                      const bufferPtr data) override {}

    void setBufferBindOffset(U32 buffer, U32 elementCountOffset) override {}

    void setFeedbackBuffer(U32 buffer, U32 bindPoint) override {}
    U32 getFeedbackPrimitiveCount(U8 queryID) override { return 0; }

    void incQueryQueue() override {}

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& command) override {}
};

};  // namespace Divide

#endif
