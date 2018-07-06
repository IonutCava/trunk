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

#ifndef _GL_GENERIC_VERTEX_DATA_H
#define _GL_GENERIC_VERTEX_DATA_H

#include "Hardware/Video/Buffers/VertexBuffer/Headers/GenericVertexData.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"

class glGenericVertexData : public GenericVertexData {
    enum GVDUsage {
        GVD_USAGE_DRAW = 0,
        GVD_USAGE_FDBCK = 1,
        GVD_USAGE_PLACEHOLDER = 2
    };

public:
    glGenericVertexData();
    ~glGenericVertexData();

    void Create(U8 numBuffers = 1, U8 numQueries = 1);
    U32  GetFeedbackPrimitiveCount(U8 queryID);

    void SetBuffer(U32 buffer, size_t dataSize, void* data, bool dynamic, bool stream);
    void UpdateBuffer(U32 buffer, size_t dataSize, void* data, U32 offset, size_t currentSize, bool dynamic, bool stream);

    inline void SetAttribute(U32 index, U32 buffer, U32 divisor, size_t size, bool normalized, U32 stride, U32 offset, const GFXDataFormat& type) {
        UpdateAttribute(index, buffer, divisor, (GLsizei)size, normalized ? GL_TRUE : GL_FALSE, stride, (GLvoid*)offset, glDataFormat[type], false);
    }

    inline void SetFeedbackAttribute(U32 index, U32 buffer, U32 divisor, size_t size, bool normalized, U32 stride, U32 offset, const GFXDataFormat& type) {
        UpdateAttribute(index, buffer, divisor, (GLsizei)size, normalized ? GL_TRUE : GL_FALSE, stride, (GLvoid*)offset, glDataFormat[type], true);
    }

    inline void Draw(const PrimitiveType& type, U32 min, U32 max, U8 queryID = 0, bool drawToBuffer = false) {
        DrawInternal(type, min, max, 1, queryID, drawToBuffer);
    }

    inline void DrawInstanced(const PrimitiveType& type, U32 count, U32 min, U32 max, U8 queryID = 0, bool drawToBuffer = false) {
        DrawInternal(type, min, max, count, queryID, drawToBuffer);
    }

    inline void SetFeedbackBuffer(U32 buffer, U32 bindPoint) {
        if (!isFeedbackBuffer(buffer)){
             _feedbackBuffers.push_back(_bufferObjects[buffer]);
        }

        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _transformFeedback);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bindPoint, _bufferObjects[buffer]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    }

protected:
    void UpdateAttribute(U32 index, U32 buffer, U32 divisor, GLsizei size, GLboolean normalized, U32 stride, GLvoid* offset, GLenum type, bool feedbackAttrib);
    void DrawInternal(const PrimitiveType& type, U32 min, U32 max, U32 instanceCount, U8 queryID = 0, bool drawToBuffer = false);

    inline bool isFeedbackBuffer(U32 index){
        for (U32 handle : _feedbackBuffers)
            if (handle == _bufferObjects[index])
               return true;

        return false;
    }

    /// Just before we render the frame
    bool frameStarted(const FrameEvent& evt);
private:
    GLuint _transformFeedback;
    GLuint _numQueries;
    GLuint _vertexArray[GVD_USAGE_PLACEHOLDER];

    GLuint* _prevResult;
    GLuint* _feedbackQueries[2];
    bool*   _resultAvailable[2];
    GLuint  _currentWriteQuery;
    GLuint  _currentReadQuery;
};
#endif
