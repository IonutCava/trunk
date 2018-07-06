#include "Headers/glGenericVertexData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(glGenericVertexData, 0, 0)

glGenericVertexData::glGenericVertexData(GFXDevice& context, const U32 ringBufferLength)
    : GenericVertexData(context, ringBufferLength),
      _prevResult(nullptr)
{
    _numQueries = 0;
    _indexBuffer = 0;
    _indexBufferSize = 0;
    _indexBufferUsage = GL_NONE;
    _currentReadQuery = 0;
    _transformFeedback = 0;
    _currentWriteQuery = 0;
    _vertexArray[to_base(GVDUsage::DRAW)] = 0;
    _vertexArray[to_base(GVDUsage::FDBCK)] = 0;
    _feedbackQueries.fill(nullptr);
    _resultAvailable.fill(nullptr);
}

glGenericVertexData::~glGenericVertexData()
{
    if (!_bufferObjects.empty()) {
        for (U8 i = 0; i < _bufferObjects.size(); ++i) {
            MemoryManager::DELETE(_bufferObjects[i]);
        }
    }

    // Make sure we don't have any of our VAOs bound
    GL_API::setActiveVAO(0);
    // Delete the rendering VAO
    if (_vertexArray[to_base(GVDUsage::DRAW)] > 0) {
        GLUtil::_vaoPool.deallocate(_vertexArray[to_base(GVDUsage::DRAW)]);
    }
    // Delete the transform feedback VAO
    if (_vertexArray[to_base(GVDUsage::FDBCK)] > 0) {
        GLUtil::_vaoPool.deallocate(_vertexArray[to_base(GVDUsage::FDBCK)]);
    }
    // Make sure we don't have the indirect draw buffer bound
    // Make sure we don't have our transform feedback object bound
    GL_API::setActiveTransformFeedback(0);
    // If we have a transform feedback object, delete it
    if (_transformFeedback > 0) {
        glDeleteTransformFeedbacks(1, &_transformFeedback);
    }

    // Delete all of our query objects
    if (_numQueries > 0) {
        for (U8 i = 0; i < 2; ++i) {
            glDeleteQueries(_numQueries, _feedbackQueries[i]);
            MemoryManager::DELETE_ARRAY(_feedbackQueries[i]);
            MemoryManager::DELETE_ARRAY(_resultAvailable[i]);
        }
    }
    GLUtil::freeBuffer(_indexBuffer);
    // Delete the rest of the data
    MemoryManager::DELETE_ARRAY(_prevResult);
}

/// Create the specified number of buffers and queries and attach them to this
/// vertex data container
void glGenericVertexData::create(U8 numBuffers, U8 numQueries) {
    // Prevent double create
    assert(_bufferObjects.empty() && "glGenericVertexData error: create called with no buffers specified!");
    // Create two vertex array objects. One for rendering and one for transform feedback
    GLUtil::_vaoPool.allocate(to_base(GVDUsage::COUNT), &_vertexArray[0]);
    // Transform feedback may not be used, but it simplifies the class interface a lot
    // Create a transform feedback object
    glGenTransformFeedbacks(1, &_transformFeedback);
    // Create our buffer objects
    _bufferObjects.resize(numBuffers, nullptr);
    // Prepare our generic queries
    _numQueries = numQueries;
    for (U8 i = 0; i < 2; ++i) {
        _feedbackQueries[i] = MemoryManager_NEW GLuint[_numQueries];
        _resultAvailable[i] = MemoryManager_NEW bool[_numQueries];
    }
    // Create our generic query objects
    for (U8 i = 0; i < 2; ++i) {
        memset(_feedbackQueries[i], 0, sizeof(GLuint) * _numQueries);
        memset(_resultAvailable[i], 0, sizeof(bool) * _numQueries);
        glGenQueries(_numQueries, _feedbackQueries[i]);
    }
    // Allocate buffers for all possible data that we may use with this object
    // Query results from the previous frame
    _prevResult = MemoryManager_NEW GLuint[_numQueries];
    memset(_prevResult, 0, sizeof(GLuint) * _numQueries);
}

void glGenericVertexData::incQueryQueue() {
    // Queries are double buffered to avoid stalling (should probably be triple buffered)
    if (!_bufferObjects.empty() && _doubleBufferedQuery) {
        _currentWriteQuery = (_currentWriteQuery + 1) % 2;
        _currentReadQuery = (_currentWriteQuery + 1) % 2;
    }
}

/// Bind a specific range of the transform feedback buffer for writing
/// (specified in the number of elements to offset by and the number of elements to be written)
void glGenericVertexData::bindFeedbackBufferRange(U32 buffer,
                                                  U32 elementCountOffset,
                                                  size_t elementCount) {
    // Only feedback buffers can be used with this method
    assert(isFeedbackBuffer(buffer) && "glGenericVertexData error: called bind buffer range for non-feedback buffer!");

    GL_API::setActiveTransformFeedback(_transformFeedback);

    glGenericBuffer* buff = _bufferObjects[buffer];
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,
                      feedbackBindPoint(buffer),
                      buff->bufferHandle(),
                      elementCountOffset * buff->elementSize(),
                      elementCount * buff->elementSize());
}

/// Submit a draw command to the GPU using this object and the specified command
void glGenericVertexData::draw(const GenericDrawCommand& command) {

    bool useCmdBuffer = command.isEnabledOption(GenericDrawCommand::RenderOptions::RENDER_INDIRECT);
    U32 drawBufferID = command.drawToBuffer();
    // Check if we are rendering to the screen or to a buffer
    bool feedbackActive = (drawBufferID > 0 && !_feedbackBuffers.empty());
    // Activate the appropriate vertex array object for the type of rendering we requested
    GLuint activeVAO = _vertexArray[feedbackActive ? to_base(GVDUsage::FDBCK)
                                                   : to_base(GVDUsage::DRAW)];
    // Update buffer bindings
    setBufferBindings(activeVAO);
    // Update vertex attributes if needed (e.g. if offsets changed)
    setAttributes(activeVAO, feedbackActive);

    // Delay this for as long as possible
    GL_API::setActiveVAO(activeVAO);
    // Activate transform feedback if needed
    if (feedbackActive) {
        GL_API::setActiveTransformFeedback(_transformFeedback);
        glBeginTransformFeedback(GLUtil::glPrimitiveTypeTable[to_U32(command.primitiveType())]);
        // Count the number of primitives written to the buffer
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _feedbackQueries[_currentWriteQuery][drawBufferID]);
    }

    // Submit the draw command
    GLUtil::submitRenderCommand(command, useCmdBuffer, GL_UNSIGNED_INT, _indexBuffer);

    // Deactivate transform feedback if needed
    if (feedbackActive) {
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();
        // Mark the current query as completed and ready to be retrieved
        _resultAvailable[_currentWriteQuery][command.drawToBuffer()] = true;
    }

    vectorAlg::vecSize bufferCount = _bufferObjects.size();
    for (vectorAlg::vecSize i = 0; i < bufferCount; ++i) {
        glGenericBuffer* buffer = _bufferObjects[i];
        buffer->lockData(buffer->elementCount(), 0, queueReadIndex());
    }
}

void glGenericVertexData::setIndexBuffer(U32 indicesCount, bool dynamic,  bool stream, const vectorImpl<U32>& indices) {
    if (indicesCount > 0) {
        assert(_indexBuffer == 0 && "glGenericVertexData::SetIndexBuffer error: Tried to double create index buffer!");

        _indexBufferUsage = dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW)
                                    : GL_STATIC_DRAW;
        // Generate an "Index Buffer Object"
        GLUtil::createAndAllocBuffer(
                static_cast<GLsizeiptr>(indicesCount * sizeof(GLuint)),
                _indexBufferUsage,
                _indexBuffer,
                indices.empty() ? NULL : (bufferPtr)indices.data());
        _indexBufferSize = indicesCount;
        // Assert if the IB creation failed
        assert(_indexBuffer != 0 && Locale::get(_ID("ERROR_IB_INIT")));
    } else {
        GLUtil::freeBuffer(_indexBuffer);
    }
}

void glGenericVertexData::updateIndexBuffer(const vectorImpl<U32>& indices) {
    DIVIDE_ASSERT(!indices.empty() && _indexBufferSize >= to_U32(indices.size()),
        "glGenericVertexData::UpdateIndexBuffer error: Invalid index buffer data!");

    assert(_indexBuffer != 0 && "glGenericVertexData::UpdateIndexBuffer error: no valid index buffer found!");

    glNamedBufferData(_indexBuffer,
                      static_cast<GLsizeiptr>(_indexBufferSize * sizeof(GLuint)),
                      NULL,
                      _indexBufferUsage);

    glNamedBufferSubData(_indexBuffer, 
        0, 
        static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
        (bufferPtr)(indices.data()));
}

/// Specify the structure and data of the given buffer
void glGenericVertexData::setBuffer(U32 buffer,
                                    U32 elementCount,
                                    size_t elementSize,
                                    bool useRingBuffer,
                                    const bufferPtr data,
                                    bool dynamic,
                                    bool stream) {
    // Make sure the buffer exists
    assert(buffer >= 0 && buffer < _bufferObjects.size() &&
           "glGenericVertexData error: set buffer called for invalid buffer index!");

    assert(_bufferObjects[buffer] == nullptr &&
           "glGenericVertexData::setBuffer : buffer repurposing is not supported at the moment");

    BufferParams params;
    params._usage = isFeedbackBuffer(buffer) ? GL_TRANSFORM_FEEDBACK : GL_ARRAY_BUFFER;
    params._elementCount = elementCount;
    params._elementSizeInBytes = elementSize;
    params._frequency = dynamic ? stream ? BufferUpdateFrequency::OFTEN
                                         : BufferUpdateFrequency::OCASSIONAL
                                : BufferUpdateFrequency::ONCE;
    params._ringSizeFactor = useRingBuffer ? queueLength() : 1;
    params._data = data;

    glGenericBuffer* tempBuffer = MemoryManager_NEW glGenericBuffer(params);
    _bufferObjects[buffer] = tempBuffer;

    // if "setFeedbackBuffer" was called before "create"
    if (isFeedbackBuffer(buffer)) {
        GL_API::setActiveTransformFeedback(_transformFeedback);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBindPoint(buffer), tempBuffer->bufferHandle());
    }
}

/// Update the elementCount worth of data contained in the buffer starting from
/// elementCountOffset size offset
void glGenericVertexData::updateBuffer(U32 buffer,
                                       U32 elementCount,
                                       U32 elementCountOffset,
                                       const bufferPtr data) {
    _bufferObjects[buffer]->updateData(elementCount, elementCountOffset, queueWriteIndex(), data);
}

void glGenericVertexData::setBufferBindOffset(U32 buffer, U32 elementCountOffset) {
    _bufferObjects[buffer]->setBindOffset(elementCountOffset);
}

void glGenericVertexData::setBufferBindings(GLuint activeVAO) {
    if (!_bufferObjects.empty()) {
        for (U32 i = 0; i < _bufferObjects.size(); ++i) {
            glGenericBuffer* buffer = _bufferObjects[i];
            GL_API::bindActiveBuffer(activeVAO,
                                     i,
                                     buffer->bufferHandle(),
                                     buffer->getBindOffset(queueReadIndex()),
                                     static_cast<GLsizei>(buffer->elementSize()));
        }
    }

}

/// Update the appropriate attributes (either for drawing or for transform feedback)
void glGenericVertexData::setAttributes(GLuint activeVAO, bool feedbackPass) {
    // Get the appropriate list of attributes
    attributeMap& map = feedbackPass ? _attributeMapFdbk : _attributeMapDraw;
    // And update them in turn
    for (attributeMap::value_type& it : map) {
        setAttributeInternal(activeVAO, it.second);
    }
}

/// Update internal attribute data
void glGenericVertexData::setAttributeInternal(GLuint activeVAO, AttributeDescriptor& descriptor) {
    // Early out if the attribute didn't change
    if (!descriptor.dirty()) {
        return;
    }

    // If the attribute wasn't activate until now, enable it
    if (!descriptor.wasSet()) {
        assert(descriptor.attribIndex() < to_U32(GL_API::s_maxAttribBindings) &&
               "GL Wrapper: insufficient number of attribute binding locations available on current hardware!");

        glEnableVertexArrayAttrib(activeVAO, descriptor.attribIndex());
        glVertexArrayAttribBinding(activeVAO, descriptor.attribIndex(), descriptor.bufferIndex());
        descriptor.wasSet(true);
    }

    // Update the attribute data
    GFXDataFormat format = descriptor.dataType();

    bool isIntegerType = format != GFXDataFormat::FLOAT_16 &&
                         format != GFXDataFormat::FLOAT_32;
    
    if (!isIntegerType || descriptor.normalized()) {
        glVertexArrayAttribFormat(activeVAO,
                                  descriptor.attribIndex(),
                                  descriptor.componentsPerElement(),
                                  GLUtil::glDataFormat[to_U32(format)],
                                  descriptor.normalized() ? GL_TRUE : GL_FALSE,
                                  to_U32(descriptor.offset() * dataSizeForType(format)));
    } else {
        glVertexArrayAttribIFormat(activeVAO,
                                   descriptor.attribIndex(),
                                   descriptor.componentsPerElement(),
                                   GLUtil::glDataFormat[to_U32(format)],
                                   to_U32(descriptor.offset() * dataSizeForType(format)));
    }

    glVertexArrayBindingDivisor(activeVAO, descriptor.bufferIndex(), descriptor.instanceDivisor());

    // Inform the descriptor that the data was updated
    descriptor.clean();
}

/// Return the number of primitives written to a transform feedback buffer that
/// used the specified query ID
U32 glGenericVertexData::getFeedbackPrimitiveCount(U8 queryID) {
    assert(queryID < _numQueries && !_bufferObjects.empty() &&
           "glGenericVertexData error: Current object isn't ready for query processing!");
    // Double buffered queries return the results from the previous draw call to
    // avoid stalling the GPU pipeline
    U32 queryEntry = _doubleBufferedQuery ? _currentReadQuery : _currentWriteQuery;
    // If the requested query has valid data available, retrieve that data from
    // the GPU
    if (_resultAvailable[queryEntry][queryID]) {
        // Get the result of the previous query about the generated primitive
        // count
        glGetQueryObjectuiv(_feedbackQueries[queryEntry][queryID],
                            GL_QUERY_RESULT, &_prevResult[queryID]);
        // Mark the query entry data as invalid now
        _resultAvailable[queryEntry][queryID] = false;
    }
    // Return either the previous value or the current one depending on the
    // previous check
    return _prevResult[queryID];
}

void glGenericVertexData::setFeedbackBuffer(U32 buffer, U32 bindPoint) {
    if (!isFeedbackBuffer(buffer)) {
        _feedbackBuffers.emplace_back(std::make_pair(buffer, bindPoint));
    }

    // if "setFeedbackBuffer" was called after "create"
    if (_bufferObjects[buffer] != nullptr) {
        GL_API::setActiveTransformFeedback(_transformFeedback);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bindPoint, _bufferObjects[buffer]->bufferHandle());
    }
}

bool glGenericVertexData::isFeedbackBuffer(U32 index) {
    for (std::pair<U32, U32>& entry : _feedbackBuffers) {
        if (entry.first == index) {
            return true;
        }
    }

    return false;
}

U32 glGenericVertexData::feedbackBindPoint(U32 buffer) {
    for (std::pair<U32, U32>& entry : _feedbackBuffers) {
        if (entry.first == buffer) {
            return entry.second;
        }
    }
    return 0;
}
};
