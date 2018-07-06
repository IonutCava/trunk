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

#ifndef _GL_GENERIC_VERTEX_DATA_H
#define _GL_GENERIC_VERTEX_DATA_H

#include "Platform/Video/Buffers/VertexBuffer/Headers/GenericVertexData.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"

namespace Divide {

class glGenericVertexData : public GenericVertexData {
    enum class GVDUsage : U32 {
        DRAW = 0,
        FDBCK = 1,
        COUNT
    };

   public:
    glGenericVertexData(bool persistentMapped);
    ~glGenericVertexData();

    void create(U8 numBuffers = 1, U8 numQueries = 1);
    U32 getFeedbackPrimitiveCount(U8 queryID);

    void setIndexBuffer(U32 indicesCount, bool dynamic,  bool stream);
    void updateIndexBuffer(const vectorImpl<U32>& indices);

    void setBuffer(U32 buffer, U32 elementCount, size_t elementSize,
                   U8 sizeFactor, void* data, bool dynamic, bool stream,
                   bool persistentMapped = false);

    void updateBuffer(U32 buffer, U32 elementCount, U32 elementCountOffset,  void* data);

    void bindFeedbackBufferRange(U32 buffer, U32 elementCountOffset, size_t elementCount);

    inline void setFeedbackBuffer(U32 buffer, U32 bindPoint) {
        if (!isFeedbackBuffer(buffer)) {
            _feedbackBuffers.push_back(_bufferObjects[buffer]._id);
            _fdbkBindPoints.push_back(bindPoint);
        }

        GL_API::setActiveTransformFeedback(_transformFeedback);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bindPoint, _bufferObjects[buffer]._id);
    }

   protected:
    friend class GFXDevice;
    void draw(const GenericDrawCommand& command,
              bool useCmdBuffer = false);

   protected:
    void setAttributes(bool feedbackPass);
    void setAttributeInternal(AttributeDescriptor& descriptor);

    inline bool isFeedbackBuffer(U32 index) {
        for (U32 handle : _feedbackBuffers)
            if (handle == _bufferObjects[index]._id) {
                return true;
            }
        return false;
    }

    inline U32 getBindPoint(U32 bufferHandle) {
        for (U8 i = 0; i < _feedbackBuffers.size(); ++i) {
            if (_feedbackBuffers[i] == bufferHandle) return _fdbkBindPoints[i];
        }
        return _fdbkBindPoints[0];
    }

    /// Just before we render the frame
    bool frameStarted(const FrameEvent& evt);

   private:
    GLuint _indexBuffer;
    GLuint _indexBufferSize;
    GLenum _indexBufferUsage;
    GLuint _transformFeedback;
    GLuint _numQueries;
    bool* _bufferSet;
    bool* _bufferPersistent;
    GLuint* _elementCount;
    GLuint* _sizeFactor;
    size_t* _readOffset;
    size_t* _elementSize;
    bufferPtr* _bufferPersistentData;
    GLuint* _prevResult;
    std::array<GLuint*, 2> _feedbackQueries;
    std::array<bool*, 2> _resultAvailable;
    GLuint _currentWriteQuery;
    GLuint _currentReadQuery;
    size_t* _startDestOffset;
    vectorImpl<U32> _fdbkBindPoints;
    vectorImpl<GLUtil::AllocationHandle> _bufferObjects;
    std::array<GLuint, to_const_uint(GVDUsage::COUNT)> _vertexArray;
    vectorImpl<std::unique_ptr<glBufferLockManager> > _lockManagers;
};

};  // namespace Divide
#endif
