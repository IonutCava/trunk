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

#ifndef _GL_GENERIC_VERTEX_DATA_H
#define _GL_GENERIC_VERTEX_DATA_H

#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glGenericBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"

namespace Divide {

class glGenericVertexData : public GenericVertexData {
    USE_CUSTOM_ALLOCATOR
    enum class GVDUsage : U32 {
        DRAW = 0,
        FDBCK = 1,
        COUNT
    };

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
    glGenericVertexData(GFXDevice& context, const U32 ringBufferLength);
    ~glGenericVertexData();

    void create(U8 numBuffers = 1, U8 numQueries = 1) override;
    U32 getFeedbackPrimitiveCount(U8 queryID) override;

    void setIndexBuffer(U32 indicesCount, bool dynamic,  bool stream, const vectorImpl<U32>& indices) override;
    void updateIndexBuffer(const vectorImpl<U32>& indices) override;

    void setBuffer(U32 buffer,
                   U32 elementCount,
                   size_t elementSize,
                   bool useRingBuffer,
                   const bufferPtr data,
                   bool dynamic,
                   bool stream) override;

    void updateBuffer(U32 buffer, U32 elementCount, U32 elementCountOffset, const bufferPtr data) override;

    void setBufferBindOffset(U32 buffer, U32 elementCountOffset) override;

    void bindFeedbackBufferRange(U32 buffer, U32 elementCountOffset, size_t elementCount) override;

    void setFeedbackBuffer(U32 buffer, U32 bindPoint) override;

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& command) override;

   protected:
    void setBufferBindings(GLuint activeVAO);
    void setAttributes(GLuint activeVAO, bool feedbackPass);
    void setAttributeInternal(GLuint activeVAO, AttributeDescriptor& descriptor);

    bool isFeedbackBuffer(U32 index);
    U32 feedbackBindPoint(U32 buffer);

    void incQueryQueue() override;

    static bool setIfDifferentBindRange(GLuint activeVAO, GLuint bindIndex, const BufferBindConfig& bindConfig);

   private:
    GLuint _indexBuffer;
    GLuint _indexBufferSize;
    GLenum _indexBufferUsage;
    GLuint _transformFeedback;
    GLuint _numQueries;
    GLuint* _prevResult;
    std::array<GLuint*, 2> _feedbackQueries;
    std::array<bool*, 2> _resultAvailable;
    GLuint _currentWriteQuery;
    GLuint _currentReadQuery;
    vectorImpl<glGenericBuffer*> _bufferObjects;
    std::array<GLuint, to_const_uint(GVDUsage::COUNT)> _vertexArray;
};

};  // namespace Divide
#endif
