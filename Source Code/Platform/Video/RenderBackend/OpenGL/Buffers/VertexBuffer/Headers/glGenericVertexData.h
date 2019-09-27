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
#ifndef _GL_GENERIC_VERTEX_DATA_H
#define _GL_GENERIC_VERTEX_DATA_H

#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glGenericBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferLockManager.h"

namespace Divide {

class glGenericVertexData final : public GenericVertexData {
    struct BufferBindConfig {
        BufferBindConfig() : BufferBindConfig(0, 0, 0)
        {
        }

        BufferBindConfig(GLuint buffer,
                         GLintptr offset,
                         GLsizei stride) : _buffer(buffer),
                                           _offset(offset),
                                           _stride(stride)
        {
        }

        inline void set(const BufferBindConfig& other) {
            _buffer = other._buffer;
            _offset = other._offset;
            _stride = other._stride;
        }

        bool operator==(const BufferBindConfig& other) const {
            return _buffer == other._buffer &&
                   _offset == other._offset &&
                   _stride == other._stride;
        }

        GLsizei  _stride;
        GLuint   _buffer;
        GLintptr _offset;
    };

   public:
    glGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name = nullptr);
    ~glGenericVertexData();

    void create(U8 numBuffers = 1) override;

    void setIndexBuffer(const IndexBuffer& indices, BufferUpdateFrequency updateFrequency) override;

    void updateIndexBuffer(const IndexBuffer& indices) override;

    void setBuffer(const SetBufferParams& params) override;

    void updateBuffer(U32 buffer, U32 elementCount, U32 elementCountOffset, const bufferPtr data) override;

    void setBufferBindOffset(U32 buffer, U32 elementCountOffset) override;

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) override;

   protected:
    void setBufferBindings();
    void setAttributes();
    void setAttributeInternal(AttributeDescriptor& descriptor);
    //HACK: Copied from glVertexArray. Move this somewhere common for both
    void rebuildCountAndIndexData(U32 drawCount, U32 indexCount, U32 firstIndex);

   private:
    bool _smallIndices;
    bool _idxBufferDirty;

    //HACK: Copied from glVertexArray. Move this somewhere common for both
    GLuint _lastDrawCount = 0;
    GLuint _lastIndexCount = 0;
    GLuint _lastFirstIndex = 0;
    std::array<GLsizei, Config::MAX_VISIBLE_NODES> _countData;
    eastl::fixed_vector<GLuint, Config::MAX_VISIBLE_NODES * 256> _indexOffsetData;

    GLuint _indexBuffer;
    GLuint _indexBufferSize;
    GLenum _indexBufferUsage;
    GLuint _vertexArray;
    vector<glGenericBuffer*> _bufferObjects;
    vector<U32> _instanceDivisor;
};

};  // namespace Divide
#endif
