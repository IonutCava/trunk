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

#ifndef _D3D_GENERIC_VERTEX_DATA_H
#define _D3d_GENERIC_VERTEX_DATA_H

#include "Hardware/Video/Buffers/VertexBuffer/Headers/GenericVertexData.h"

class d3dGenericVertexData : public GenericVertexData {

public:
    d3dGenericVertexData(bool persistentMapped) : GenericVertexData(persistentMapped)
    {
    }
    ~d3dGenericVertexData()
    {
    }

    void Create(U8 numBuffers = 1, U8 numQueries = 1)
    {
    }
    void Draw(const PrimitiveType& type, U32 min, U32 max, U8 queryID = 0, bool drawToBuffer = false)
    {
    }
    void DrawInstanced(const PrimitiveType& type, U32 count, U32 min, U32 max, U8 queryID = 0, bool drawToBuffer = false)
    {
    }
    void SetBuffer(U32 buffer, U32 elementCount, size_t elementSize, void* data, bool dynamic, bool stream, bool persistentMapped = false)
    {
    }
    void BindFeedbackBufferRange(U32 buffer, size_t elementCountOffset, size_t elementCount)
    {
    }
    void UpdateBuffer(U32 buffer, U32 elementCount, void* data, U32 offset, bool dynamic, bool steam)
    {
    }
    void SetFeedbackBuffer(U32 buffer, U32 bindPoint)
    {
    }
    U32  GetFeedbackPrimitiveCount(U8 queryID)
    {
        return 0;
    }
};
#endif
