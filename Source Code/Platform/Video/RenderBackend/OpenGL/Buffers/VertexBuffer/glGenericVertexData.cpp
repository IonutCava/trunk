#include "stdafx.h"

#include "Headers/glGenericVertexData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(glGenericVertexData, 0, 0)

glGenericVertexData::glGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
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
    _smallIndices = false;
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
        GL_API::s_vaoPool.deallocate(_vertexArray[to_base(GVDUsage::DRAW)]);
    }
    // Delete the transform feedback VAO
    if (_vertexArray[to_base(GVDUsage::FDBCK)] > 0) {
        GL_API::s_vaoPool.deallocate(_vertexArray[to_base(GVDUsage::FDBCK)]);
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
    GL_API::s_vaoPool.allocate(to_base(GVDUsage::COUNT), &_vertexArray[0]);
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
                      elementCountOffset * buff->bufferImpl()->elementSize(),
                      elementCount * buff->bufferImpl()->elementSize());
}

/// Submit a draw command to the GPU using this object and the specified command
void glGenericVertexData::draw(const GenericDrawCommand& command) {
    bool useCmdBuffer = isEnabledOption(command, CmdRenderOptions::RENDER_INDIRECT);
    U32 drawBufferID = command._bufferIndex;
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
        glBeginTransformFeedback(GLUtil::glPrimitiveTypeTable[to_U32(command._primitiveType)]);
        // Count the number of primitives written to the buffer
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _feedbackQueries[_currentWriteQuery][drawBufferID]);
    }

    // Submit the draw command
    GLUtil::submitRenderCommand(command, _indexBuffer > 0, useCmdBuffer, _smallIndices ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);

    // Deactivate transform feedback if needed
    if (feedbackActive) {
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();
        // Mark the current query as completed and ready to be retrieved
        _resultAvailable[_currentWriteQuery][drawBufferID] = true;
    }

    vec_size bufferCount = _bufferObjects.size();
    for (vec_size i = 0; i < bufferCount; ++i) {
        glGenericBuffer* buffer = _bufferObjects[i];
        buffer->lockData(buffer->elementCount(), 0, queueIndex(), false);
    }
}

void glGenericVertexData::setIndexBuffer(const IndexBuffer& indices, BufferUpdateFrequency updateFrequency) {
    if (_indexBuffer != 0) {
        GLUtil::freeBuffer(_indexBuffer);
    }

    if (indices.count > 0) {
        _indexBufferUsage = 
            updateFrequency == BufferUpdateFrequency::ONCE
                             ? GL_STATIC_DRAW
                             : updateFrequency == BufferUpdateFrequency::OCASSIONAL
                                                ? GL_DYNAMIC_DRAW
                                                : GL_STREAM_DRAW;
        // Generate an "Index Buffer Object"
        _indexBufferSize = (GLuint)(indices.count * (indices.smallIndices ? sizeof(U16) : sizeof(U32)));
        _smallIndices = indices.smallIndices;

        GLUtil::createAndAllocBuffer(
            _indexBufferSize,
            _indexBufferUsage,
            _indexBuffer,
            indices.data,
            _name.empty() ? nullptr : (_name + "_index").c_str());
    }

    GL_API::setActiveVAO(_vertexArray[to_base(GVDUsage::DRAW)]);
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
}

void glGenericVertexData::updateIndexBuffer(const IndexBuffer& indices) {
    DIVIDE_ASSERT(indices.count > 0, "glGenericVertexData::UpdateIndexBuffer error: Invalid index buffer data!");
    assert(_indexBuffer != 0 && "glGenericVertexData::UpdateIndexBuffer error: no valid index buffer found!");

    size_t elementSize = indices.smallIndices ? sizeof(GLushort) : sizeof(GLuint);

    if (indices.offsetCount == 0) {
        _indexBufferSize = (GLuint)(indices.count * elementSize);

        glInvalidateBufferData(_indexBuffer);
        glNamedBufferData(_indexBuffer,
                          _indexBufferSize,
                          indices.data,
                          _indexBufferUsage);
    } else {
        size_t size = (indices.count + indices.offsetCount) * elementSize;
        DIVIDE_ASSERT(size < _indexBufferSize);
        glInvalidateBufferSubData(_indexBuffer,
                                  indices.offsetCount * elementSize,
                                  indices.count * elementSize);
        glNamedBufferSubData(_indexBuffer,
                             indices.offsetCount * elementSize,
                             indices.count * elementSize,
                             indices.data);
    }
}

/// Specify the structure and data of the given buffer
void glGenericVertexData::setBuffer(U32 buffer,
                                    U32 elementCount,
                                    size_t elementSize,
                                    bool useRingBuffer,
                                    const bufferPtr data,
                                    BufferUpdateFrequency updateFrequency) {
    // Make sure the buffer exists
    assert(buffer >= 0 && buffer < _bufferObjects.size() &&
           "glGenericVertexData error: set buffer called for invalid buffer index!");

    assert(_bufferObjects[buffer] == nullptr &&
           "glGenericVertexData::setBuffer : buffer re-purposing is not supported at the moment");

    BufferParams params;
    params._usage = isFeedbackBuffer(buffer) ? GL_TRANSFORM_FEEDBACK : GL_ARRAY_BUFFER;
    params._elementCount = elementCount;
    params._elementSizeInBytes = elementSize;
    params._frequency = updateFrequency;
    params._ringSizeFactor = useRingBuffer ? queueLength() : 1;
    params._data = data;
    params._name = _name.empty() ? nullptr : _name.c_str();
    glGenericBuffer* tempBuffer = MemoryManager_NEW glGenericBuffer(_context, params);
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
    _bufferObjects[buffer]->writeData(elementCount, elementCountOffset, queueIndex(), data);
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
                                     buffer->getBindOffset(queueIndex()),
                                     static_cast<GLsizei>(buffer->bufferImpl()->elementSize()));
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
                                  (GLuint)descriptor.strideInBytes());
    } else {
        glVertexArrayAttribIFormat(activeVAO,
                                   descriptor.attribIndex(),
                                   descriptor.componentsPerElement(),
                                   GLUtil::glDataFormat[to_U32(format)],
                                   (GLuint)descriptor.strideInBytes());
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
