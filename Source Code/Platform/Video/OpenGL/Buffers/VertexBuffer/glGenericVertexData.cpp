#include "Headers/glGenericVertexData.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

hashMapImpl<GLuint, glGenericVertexData::BufferBindConfig> glGenericVertexData::_bindConfigs;

IMPLEMENT_CUSTOM_ALLOCATOR(glGenericVertexData, 0, 0)
bool glGenericVertexData::setIfDifferentBindRange(GLuint bindIndex, const BufferBindConfig& bindConfig) {

    // If this is a new index, this will just create a default config
    BufferBindConfig& crtConfig = _bindConfigs[bindIndex];
    if (bindConfig == crtConfig) {
        return false;
    }

    crtConfig.set(bindConfig);
    glBindVertexBuffer(bindIndex, bindConfig._buffer, bindConfig._offset, bindConfig._stride);
    return true;
}

glGenericVertexData::glGenericVertexData(GFXDevice& context, const U32 ringBufferLength)
    : GenericVertexData(context, ringBufferLength),
      _bufferSet(nullptr),
      _bufferIsRing(nullptr),
      _elementCount(nullptr),
      _elementSize(nullptr),
      _prevResult(nullptr)
{
    _numQueries = 0;
    _indexBuffer = 0;
    _indexBufferSize = 0;
    _indexBufferUsage = GL_NONE;
    _currentReadQuery = 0;
    _transformFeedback = 0;
    _currentWriteQuery = 0;
    _vertexArray[to_const_uint(GVDUsage::DRAW)] = 0;
    _vertexArray[to_const_uint(GVDUsage::FDBCK)] = 0;
    _feedbackQueries.fill(nullptr);
    _resultAvailable.fill(nullptr);
}

glGenericVertexData::~glGenericVertexData() {
    if (!_bufferObjects.empty()) {
        for (U8 i = 0; i < _bufferObjects.size(); ++i) {
            _bufferObjects[i]->destroy();
            MemoryManager::DELETE(_bufferObjects[i]);
        }
    }

    // Make sure we don't have any of our VAOs bound
    GL_API::setActiveVAO(0);
    // Delete the rendering VAO
    if (_vertexArray[to_const_uint(GVDUsage::DRAW)] > 0) {
        glDeleteVertexArrays(1, &_vertexArray[to_const_uint(GVDUsage::DRAW)]);
        _vertexArray[to_const_uint(GVDUsage::DRAW)] = 0;
    }
    // Delete the transform feedback VAO
    if (_vertexArray[to_const_uint(GVDUsage::FDBCK)] > 0) {
        glDeleteVertexArrays(1, &_vertexArray[to_const_uint(GVDUsage::FDBCK)]);
        _vertexArray[to_const_uint(GVDUsage::FDBCK)] = 0;
    }
    // Make sure we don't have the indirect draw buffer bound
    // Make sure we don't have our transform feedback object bound
    GL_API::setActiveTransformFeedback(0);
    // If we have a transform feedback object, delete it
    if (_transformFeedback > 0) {
        glDeleteTransformFeedbacks(1, &_transformFeedback);
        _transformFeedback = 0;
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
    _indexBufferSize = 0;
    // Delete the rest of the data
    MemoryManager::DELETE_ARRAY(_prevResult);
    MemoryManager::DELETE_ARRAY(_bufferSet);
    MemoryManager::DELETE_ARRAY(_bufferIsRing);
    MemoryManager::DELETE_ARRAY(_elementCount);
    MemoryManager::DELETE_ARRAY(_elementSize);

    _bufferObjects.clear();
    _feedbackBuffers.clear();
}

/// Create the specified number of buffers and queries and attach them to this
/// vertex data container
void glGenericVertexData::create(U8 numBuffers, U8 numQueries) {
    // Prevent double create
    DIVIDE_ASSERT(
        _bufferObjects.empty(),
        "glGenericVertexData error: create called with no buffers specified!");
    // Create two vertex array objects. One for rendering and one for transform
    // feedback
    glGenVertexArrays(to_const_uint(GVDUsage::COUNT), &_vertexArray[0]);
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
        memset(_resultAvailable[i], false, sizeof(bool) * _numQueries);
        glGenQueries(_numQueries, _feedbackQueries[i]);
    }
    // Allocate buffers for all possible data that we may use with this object
    // Query results from the previous frame
    _prevResult = MemoryManager_NEW GLuint[_numQueries];
    memset(_prevResult, 0, sizeof(GLuint) * _numQueries);
    // Flags to verify if each buffer was created
    _bufferSet = MemoryManager_NEW bool[numBuffers];
    memset(_bufferSet, false, numBuffers * sizeof(bool));
    // The element count for each buffer
    _elementCount = MemoryManager_NEW GLuint[numBuffers];
    memset(_elementCount, 0, numBuffers * sizeof(GLuint));
    // The element size for each buffer (in bytes)
    _elementSize = MemoryManager_NEW size_t[numBuffers];
    memset(_elementSize, 0, numBuffers * sizeof(size_t));
    // A flag to check if the buffer uses the RingBuffer system
    _bufferIsRing = MemoryManager_NEW bool[numBuffers];
    memset(_bufferIsRing, false, numBuffers * sizeof(bool));
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
    DIVIDE_ASSERT(isFeedbackBuffer(buffer),
                  "glGenericVertexData error: called bind buffer range for "
                  "non-feedback buffer!");

    GL_API::setActiveTransformFeedback(_transformFeedback);
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,
                      getBindPoint(_bufferObjects[buffer]->bufferID()),
                      _bufferObjects[buffer]->bufferID(),
                      elementCountOffset * _elementSize[buffer],
                      elementCount * _elementSize[buffer]);
}

/// Submit a draw command to the GPU using this object and the specified command
void glGenericVertexData::draw(const GenericDrawCommand& command, bool useCmdBuffer) {

    U32 drawBufferID = command.drawToBuffer();
    // Check if we are rendering to the screen or to a buffer
    bool feedbackActive = (drawBufferID > 0 && !_feedbackBuffers.empty());
    // Activate the appropriate vertex array object for the type of rendering we
    // requested
    GL_API::setActiveVAO(_vertexArray[feedbackActive ? to_const_uint(GVDUsage::FDBCK) 
                                                     : to_const_uint(GVDUsage::DRAW)]);
    // Update vertex attributes if needed (e.g. if offsets changed)
    setAttributes(feedbackActive);

    // Activate transform feedback if needed
    if (feedbackActive) {
        GL_API::setActiveTransformFeedback(_transformFeedback);
        glBeginTransformFeedback(GLUtil::glPrimitiveTypeTable[to_uint(command.primitiveType())]);
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
        size_t range = _elementCount[i] * _elementSize[i];
        size_t offset = 0;
        if (_bufferIsRing[i]) {
            offset += range * queueReadIndex();
        }
        _bufferObjects[i]->lockRange(static_cast<GLuint>(offset), static_cast<GLuint>(range));
    }
}

void glGenericVertexData::setIndexBuffer(U32 indicesCount, bool dynamic,  bool stream) {
    if (indicesCount > 0) {
        DIVIDE_ASSERT(_indexBuffer == 0,
            "glGenericVertexData::SetIndexBuffer error: Tried to double create index buffer!");

        _indexBufferUsage = dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW)
                                    : GL_STATIC_DRAW;
        // Generate an "Index Buffer Object"
        GLUtil::createAndAllocBuffer(
                static_cast<GLsizeiptr>(indicesCount * sizeof(GLuint)),
                _indexBufferUsage,
                _indexBuffer,
                NULL);
        _indexBufferSize = indicesCount;
        // Assert if the IB creation failed
        DIVIDE_ASSERT(_indexBuffer != 0, Locale::get(_ID("ERROR_IB_INIT")));
    } else {
        GLUtil::freeBuffer(_indexBuffer);
    }
}

void glGenericVertexData::updateIndexBuffer(const vectorImpl<U32>& indices) {
    DIVIDE_ASSERT(!indices.empty() && _indexBufferSize >= to_uint(indices.size()),
        "glGenericVertexData::UpdateIndexBuffer error: Invalid index buffer data!");

    DIVIDE_ASSERT(_indexBuffer != 0,
        "glGenericVertexData::UpdateIndexBuffer error: no valid index buffer found!");

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
                                    void* data,
                                    bool dynamic,
                                    bool stream,
                                    bool persistentMapped) {
    // Make sure the buffer exists
    DIVIDE_ASSERT(buffer >= 0 && buffer < _bufferObjects.size(),
                  "glGenericVertexData error: set buffer called for invalid "
                  "buffer index!");
    // glBufferData on persistentMapped buffers is not allowed
    DIVIDE_ASSERT(!_bufferSet[buffer],
                  "glGenericVertexData error: set buffer called for an already "
                  "created buffer!");
    // Make sure we allow persistent mapping
    if (persistentMapped) {
        persistentMapped = !Config::Profile::DISABLE_PERSISTENT_BUFFER;
    }

    // Remember the element count and size for the current buffer
    _elementCount[buffer] = elementCount;
    _elementSize[buffer] = elementSize;
    _bufferIsRing[buffer] = useRingBuffer;

    size_t bufferSize = elementCount * elementSize;
    GLuint sizeFactor = useRingBuffer ? queueLength() : 1;
    size_t totalSize = bufferSize * sizeFactor;

    assert(_bufferObjects[buffer] == nullptr &&
           "glGenericVertexData::setBuffer : buffer repurposing is not supported at the moment");

    GLenum usage = isFeedbackBuffer(buffer) ? GL_TRANSFORM_FEEDBACK : GL_BUFFER;
    if (persistentMapped) {
        _bufferObjects[buffer] = MemoryManager_NEW glPersistentBuffer(usage);
    } else {
        _bufferObjects[buffer] = MemoryManager_NEW glRegularBuffer(usage);
    }

    glBufferImpl* bufferObj = _bufferObjects[buffer];

    BufferUpdateFrequency frequency = dynamic ? stream ? BufferUpdateFrequency::OFTEN 
                                                       : BufferUpdateFrequency::OCASSIONAL
                                              : BufferUpdateFrequency::ONCE;
    bufferObj->create(frequency, totalSize);

    // Create sizeFactor copies of the data and store them in the buffer
    if (data != nullptr) {
        for (U8 i = 0; i < sizeFactor; ++i) {
            bufferObj->updateData(i * bufferSize, bufferSize, data);
        }
    }

    _bufferSet[buffer] = true;
}

/// Update the elementCount worth of data contained in the buffer starting from
/// elementCountOffset size offset
void glGenericVertexData::updateBuffer(U32 buffer,
                                       U32 elementCount,
                                       U32 elementCountOffset,
                                       void* data) {
    // Calculate the size of the data that needs updating
    size_t dataCurrentSize = elementCount * _elementSize[buffer];
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offset = elementCountOffset * _elementSize[buffer];

    if (_bufferIsRing[buffer]) {
        offset += _elementCount[buffer] * _elementSize[buffer] * queueWriteIndex();
    }

    _bufferObjects[buffer]->updateData(offset, dataCurrentSize, data);
}

/// Update the appropriate attributes (either for drawing or for transform
/// feedback)
void glGenericVertexData::setAttributes(bool feedbackPass) {
    // Get the appropriate list of attributes
    attributeMap& map = feedbackPass ? _attributeMapFdbk : _attributeMapDraw;
    // And update them in turn
    for (attributeMap::value_type& it : map) {
        setAttributeInternal(it.second);
    }
}

/// Update internal attribute data
void glGenericVertexData::setAttributeInternal(AttributeDescriptor& descriptor) {
    U32 bufferIndex = descriptor.bufferIndex();

    GLintptr offset = descriptor.offset() * _elementSize[bufferIndex];
    if (_bufferIsRing[bufferIndex]) {
        offset += _elementCount[bufferIndex] * _elementSize[bufferIndex] * queueReadIndex();
    }

    BufferBindConfig crtConfig = BufferBindConfig(_bufferObjects[bufferIndex]->bufferID(),
                                                  offset,
                                                  static_cast<GLsizei>(_elementSize[bufferIndex]));

    setIfDifferentBindRange(bufferIndex, crtConfig);

    // Early out if the attribute didn't change
    if (!descriptor.dirty()) {
        return;
    }

    // If the attribute wasn't activate until now, enable it
    if (!descriptor.wasSet()) {
        glEnableVertexAttribArray(descriptor.attribIndex());
        glVertexAttribBinding(descriptor.attribIndex(), bufferIndex);
        descriptor.wasSet(true);
    }
    // Update the attribute data

    GFXDataFormat format = descriptor.dataType();

    bool isIntegerType = format != GFXDataFormat::FLOAT_16 &&
                         format != GFXDataFormat::FLOAT_32;
    
    if (!isIntegerType || descriptor.normalized()) {
        glVertexAttribFormat(descriptor.attribIndex(),
                             descriptor.componentsPerElement(),
                             GLUtil::glDataFormat[to_uint(format)],
                             descriptor.normalized() ? GL_TRUE : GL_FALSE,
                             0);
    } else {
        glVertexAttribIFormat(descriptor.attribIndex(),
                              descriptor.componentsPerElement(),
                              GLUtil::glDataFormat[to_uint(format)],
                              0);
    }

    if (descriptor.instanceDivisor() > 0) {
        glVertexBindingDivisor(descriptor.attribIndex(), descriptor.instanceDivisor());
    }

    // Inform the descriptor that the data was updated
    descriptor.clean();
}

/// Return the number of primitives written to a transform feedback buffer that
/// used the specified query ID
U32 glGenericVertexData::getFeedbackPrimitiveCount(U8 queryID) {
    DIVIDE_ASSERT(queryID < _numQueries && !_bufferObjects.empty(),
                  "glGenericVertexData error: Current object isn't ready for "
                  "query processing!");
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
        _feedbackBuffers.push_back(_bufferObjects[buffer]->bufferID());
        _fdbkBindPoints.push_back(bindPoint);
    }

    GL_API::setActiveTransformFeedback(_transformFeedback);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bindPoint, _bufferObjects[buffer]->bufferID());
}

bool glGenericVertexData::isFeedbackBuffer(U32 index) {
    for (U32 handle : _feedbackBuffers)
        if (handle == _bufferObjects[index]->bufferID()) {
            return true;
        }
    return false;
}

U32 glGenericVertexData::getBindPoint(U32 bufferHandle) {
    for (U8 i = 0; i < _feedbackBuffers.size(); ++i) {
        if (_feedbackBuffers[i] == bufferHandle) {
            return _fdbkBindPoints[i];
        }
    }
    return _fdbkBindPoints[0];
}
};
