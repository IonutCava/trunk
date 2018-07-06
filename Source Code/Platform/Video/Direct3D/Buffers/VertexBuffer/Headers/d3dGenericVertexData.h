/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "Platform/Video/Buffers/VertexBuffer/Headers/GenericVertexData.h"

namespace Divide {

class d3dGenericVertexData : public GenericVertexData {
   public:
    d3dGenericVertexData(GFXDevice& context, bool persistentMapped)
        : GenericVertexData(context, persistentMapped)
    {
    }

    ~d3dGenericVertexData()
    {
    }

    void create(U8 numBuffers = 1, U8 numQueries = 1) {}

    void setIndexBuffer(U32 indicesCount, bool dynamic, bool stream) {}
    void updateIndexBuffer(const vectorImpl<U32>& indices) {}

    void setBuffer(U32 buffer, U32 elementCount, size_t elementSize,
                   U8 sizeFactor, void* data, bool dynamic, bool stream,
                   bool persistentMapped = false) {}
    void bindFeedbackBufferRange(U32 buffer, U32 elementCountOffset,
                                 size_t elementCount) {}
    void updateBuffer(U32 buffer, U32 elementCount, U32 elementCountOffset,
                      void* data) {}
    void setFeedbackBuffer(U32 buffer, U32 bindPoint) {}
    U32 getFeedbackPrimitiveCount(U8 queryID) { return 0; }

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& command,
              bool useCmdBuffer = false) {}
};

};  // namespace Divide

#endif
