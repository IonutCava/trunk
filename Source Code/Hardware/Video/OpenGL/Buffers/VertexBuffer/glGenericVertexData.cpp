#include "Headers/glGenericVertexData.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

glGenericVertexData::glGenericVertexData(bool persistentMapped) : GenericVertexData(persistentMapped),
                                                                  _prevResult(nullptr),
                                                                  _bufferSet(nullptr),
                                                                  _bufferPersistent(nullptr),
                                                                  _bufferPersistentData(nullptr),
                                                                  _elementCount(nullptr),
                                                                  _elementSize(nullptr),
                                                                  _sizeFactor(nullptr),
                                                                  _startDestOffset(nullptr),
                                                                  _readOffset(nullptr),
                                                                  _lockManager(nullptr)
{
    _numQueries  = 0;
    _currentReadQuery = 0;
    _transformFeedback = 0;
    _currentWriteQuery = 0;
    _indirectDrawBuffer = 0;
    _vertexArray[GVD_USAGE_DRAW] = 0;
    _vertexArray[GVD_USAGE_FDBCK] = 0;

    for (U8 i = 0; i < 2; ++i) {
        _feedbackQueries[i] = nullptr;
        _resultAvailable[i] = nullptr;
    }

    if (Config::Profile::DISABLE_PERSISTENT_BUFFER) {
        persistentMapped = _persistentMapped = false;
    }
    // Persistently mapped buffers require explicit synchronization
    if (persistentMapped) {
        _lockManager = New glBufferLockManager(true);
    }
}

glGenericVertexData::~glGenericVertexData()
{
    if (!_bufferObjects.empty()) {
        // If we have persistently mapped buffers, we need to unmap them first
        for (U8 i = 0; i < _bufferObjects.size(); ++i) {
            if (_bufferPersistent[i]) {
                GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[i]);
                glUnmapBuffer(GL_ARRAY_BUFFER);
            }
        }
        // Make sure we don't have any of our buffers still bound
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, 0);
        // Delete the buffer objects
        glDeleteBuffers((GLsizei)_bufferObjects.size(), &_bufferObjects[0]);
    }
    // Make sure we don't have any of our VAOs bound
    GL_API::setActiveVAO(0);
    // Delete the rendering VAO
    if (_vertexArray[GVD_USAGE_DRAW] > 0) {
        glDeleteVertexArrays(GVD_USAGE_DRAW, &_vertexArray[0]);
        _vertexArray[GVD_USAGE_DRAW] = 0;
    }
    // Delete the transform feedback VAO
    if (_vertexArray[GVD_USAGE_FDBCK] > 0) {
        glDeleteVertexArrays(GVD_USAGE_FDBCK, &_vertexArray[0]);
        _vertexArray[GVD_USAGE_FDBCK] = 0;
    }
    // Make sure we don't have the indirect draw buffer bound
    //glBindBuffer(GL_QUERY_BUFFER_AMD, 0);
    // If we have an indirect draw buffer, delete it
    if (_indirectDrawBuffer > 0) {
        glDeleteBuffers(1, &_indirectDrawBuffer);
        _indirectDrawBuffer = 0;
    }
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
            SAFE_DELETE_ARRAY(_feedbackQueries[i]);
            SAFE_DELETE_ARRAY(_resultAvailable[i]);
        }
    }

    // Delete the rest of the data
    SAFE_DELETE_ARRAY(_prevResult);
    SAFE_DELETE_ARRAY(_bufferSet);
    SAFE_DELETE_ARRAY(_bufferPersistent);
    SAFE_DELETE_ARRAY(_elementCount);
    SAFE_DELETE_ARRAY(_elementSize);
    SAFE_DELETE_ARRAY(_sizeFactor);
    SAFE_DELETE_ARRAY(_startDestOffset);
    SAFE_DELETE_ARRAY(_readOffset);
    SAFE_DELETE(_lockManager);
    if (_bufferPersistentData != nullptr) {
        free(_bufferPersistentData);
    }

    _bufferObjects.clear();
    _feedbackBuffers.clear();
}

/// Create the specified number of buffers and queries and attach them to this vertex data container
void glGenericVertexData::Create(U8 numBuffers, U8 numQueries) {
    // Prevent double create
    DIVIDE_ASSERT(_bufferObjects.empty(), "glGenericVertexData error: create called with no buffers specified!");
    // Create two vertex array objects. One for rendering and one for transform feedback 
    glGenVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
    // Transform feedback may not be used, but it simplifies the class interface a lot
    // Create a transform feedback object
    glGenTransformFeedbacks(1, &_transformFeedback);
    // Create our buffer objects
    _bufferObjects.resize(numBuffers, 0);
    glGenBuffers(numBuffers, &_bufferObjects[0]);
    // Allocate a buffer for indirect draw used to store the query results without a round-trip to the CPU
    glGenBuffers(1, &_indirectDrawBuffer);
    // Prepare our generic queries
    _numQueries = numQueries;
    for (U8 i = 0; i < 2; ++i) {
        _feedbackQueries[i] = New GLuint[_numQueries];
        _resultAvailable[i] = New bool[_numQueries];
    }
    // Create our generic query objects
    for (U8 i = 0; i < 2; ++i) {
        memset(_feedbackQueries[i], 0, sizeof(GLuint) * _numQueries);
        memset(_resultAvailable[i], false, sizeof(bool)   * _numQueries);
        glGenQueries(_numQueries, _feedbackQueries[i]);
    }
    // How many times larger should the buffer be than the actual data to offset reading and writing
    _sizeFactor = New GLuint[numBuffers];
    memset(_sizeFactor, 0, numBuffers * sizeof(GLuint));
    // Allocate buffers for all possible data that we may use with this object
    // Query results from the previous frame
    _prevResult = New GLuint[_numQueries];
    memset(_prevResult, 0, sizeof(GLuint) * _numQueries);
    // Flags to verify if each buffer was created
    _bufferSet = New bool[numBuffers];
    memset(_bufferSet, false, numBuffers * sizeof(bool));
    // The element count for each buffer
    _elementCount = New GLuint[numBuffers];
    memset(_elementCount, 0, numBuffers * sizeof(GLuint));
    // The element size for each buffer (in bytes)
    _elementSize = New size_t[numBuffers];
    memset(_elementSize, 0, numBuffers * sizeof(size_t));
    // Current buffer write head position for 3x buffer updates
    _startDestOffset = New size_t[numBuffers];
    memset(_startDestOffset, 0, numBuffers * sizeof(size_t));
    // Current buffer read head position for 3x buffer updates
    _readOffset = New size_t[numBuffers];
    memset(_readOffset, 0, numBuffers * sizeof(size_t));
    // A flag to check if the buffer is or isn't persistently mapped
    _bufferPersistent = New bool[numBuffers];
    memset(_bufferPersistent, false, numBuffers * sizeof(bool));
    // Persistently mapped data (array of void* pointers)
    _bufferPersistentData = (void**)malloc(sizeof(void*) * numBuffers);
}

/// Called at the beginning of each frame to update the currently used queries for reading and writing
bool glGenericVertexData::frameStarted(const FrameEvent& evt) {
    // Queries are double buffered to avoid stalling (should probably be triple buffered)
    if (!_bufferObjects.empty() && _doubleBufferedQuery){
        _currentWriteQuery = (_currentWriteQuery + 1) % 2;
        _currentReadQuery  = (_currentWriteQuery + 1) % 2;
    }

    return GenericVertexData::frameStarted(evt);
}

/// Bind a specific range of the transform feedback buffer for writing (specified in the number of elements to offset by and the number of elements to be written)
void glGenericVertexData::BindFeedbackBufferRange(U32 buffer, U32 elementCountOffset, size_t elementCount) {
    // Only feedback buffers can be used with this method
    DIVIDE_ASSERT(isFeedbackBuffer(buffer), "glGenericVertexData error: called bind buffer range for non-feedback buffer!");

    GL_API::setActiveTransformFeedback(_transformFeedback);
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, getBindPoint(_bufferObjects[buffer]), _bufferObjects[buffer], elementCountOffset * _elementSize[buffer], elementCount * _elementSize[buffer]);
}

/// Submit a draw command to the GPU using this object and the specified command
void glGenericVertexData::Draw(const GenericDrawCommand& command, bool skipBind) {
    // Get the OpenGL specific command from the generic one
    const IndirectDrawCommand& cmd = command.cmd();
    // Instance count can be generated programmatically, so make sure it's valid
    if (cmd.instanceCount == 0) {
        return;
    }
    // Check if we are rendering to the screen or to a buffer
    bool feedbackActive = (command.drawToBuffer() && !_feedbackBuffers.empty());
    // Activate the appropriate vertex array object for the type of rendering we requested
    GL_API::setActiveVAO(_vertexArray[feedbackActive ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);
    // Update vertex attributes if needed (e.g. if offsets changed)
    SetAttributes(feedbackActive);

    // Write query result to the primCount field of the indirect draw command to avoid a round trip to the CPU
    //glBindBuffer(GL_QUERY_BUFFER_AMD, _indirectDrawBuffer);
    //glGetQueryObjectuiv(queryId, GL_QUERY_RESULT_AVAILABLE, BUFFER_OFFSET(offsetof(DrawArraysIndirectCommand, primCount)));
    //GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectDrawBuffer);
    //glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand), &cmd, GL_DYNAMIC_COPY);

    // Activate transform feedback if needed
    if (feedbackActive) {
       GL_API::setActiveTransformFeedback(_transformFeedback);
       glBeginTransformFeedback(GLUtil::GL_ENUM_TABLE::glPrimitiveTypeTable[command.primitiveType()]);
       // Count the number of primitives written to the buffer
       glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _feedbackQueries[_currentWriteQuery][command.queryID()]);
    }
    
    // Submit the draw command
    if (!Config::Profile::DISABLE_DRAWS) {
        GLenum type = command.renderWireframe() ? GL_LINE_LOOP : GLUtil::GL_ENUM_TABLE::glPrimitiveTypeTable[command.primitiveType()];
        glDrawArraysIndirect(type, &cmd);
    }
    // Deactivate transform feedback if needed
    if (feedbackActive) {
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();
        // Mark the current query as completed and ready to be retrieved
        _resultAvailable[_currentWriteQuery][command.queryID()] = true;
    }
    // Count the draw call
    GFX_DEVICE.registerDrawCall();    
}

/// Specify the structure and data of the given buffer
void glGenericVertexData::SetBuffer(U32 buffer, U32 elementCount, size_t elementSize, U8 sizeFactor, void* data, bool dynamic, bool stream, bool persistentMapped) {
    // Make sure the buffer exists
    DIVIDE_ASSERT(buffer >= 0 && buffer < _bufferObjects.size(), "glGenericVertexData error: set buffer called for invalid buffer index!");
    // glBufferData on persistentMapped buffers is not allowed
    DIVIDE_ASSERT(!_bufferSet[buffer], "glGenericVertexData error: set buffer called for an already created buffer!");
    // Make sure we allow persistent mapping
    if (persistentMapped) {
        persistentMapped = _persistentMapped;
    }
    DIVIDE_ASSERT((persistentMapped && _persistentMapped) || !persistentMapped, "glGenericVertexData error: persistent map flag is not compatible with object details!");
    // Remember the element count and size for the current buffer
    _elementCount[buffer] = elementCount;
    _elementSize[buffer] = elementSize;
    _sizeFactor[buffer] = sizeFactor;
    size_t bufferSize = elementCount * elementSize;
    // Bind the buffer object
    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
    
    if (persistentMapped) {
        // If we requested a persistently mapped buffer, we use glBufferStorage to pin it in memory
        GLenum flag = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize * sizeFactor, NULL, flag);
        // Map the entire buffer range
        _bufferPersistentData[buffer] = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize * 3, flag);
        DIVIDE_ASSERT(data != nullptr, "glGenericVertexData error: persistent mapping failed when setting the current buffer!");
        // Create sizeFactor copies of the data and store them in the buffer
        for(U8 i = 0; i < sizeFactor; ++i) {
            U8* dst = (U8*)_bufferPersistentData[buffer] + bufferSize * i;
            memcpy(dst, data, bufferSize);
        }
        // Make sure we synchronize the write commands
        _lockManager->LockRange(0, bufferSize * sizeFactor);
    } else {
        GLenum flag = isFeedbackBuffer(buffer) ? (dynamic ? (stream ? GL_STREAM_COPY : GL_DYNAMIC_COPY) : GL_STATIC_COPY) :
                                                 (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);
        // If the buffer is not persistently mapped, allocate storage the classic way
        glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeFactor, NULL, flag);
        // And upload sizeFactor copies of the data
        for (U8 i = 0; i < sizeFactor; ++i) {
            glBufferSubData(GL_ARRAY_BUFFER, i * bufferSize, bufferSize, data);
        }
    }
    _bufferSet[buffer] = true;
    _bufferPersistent[buffer] = persistentMapped;
}

/// Update the elementCount worth of data contained in the buffer starting from elementCountOffset size offset
void glGenericVertexData::UpdateBuffer(U32 buffer, U32 elementCount, U32 elementCountOffset, void* data, bool invalidateRange) {
    // Calculate the size of the data that needs updating
    size_t dataCurrentSize = elementCount * _elementSize[buffer];
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offset = elementCountOffset * _elementSize[buffer];
    // Calculate the entire buffer size
    size_t bufferSize = _elementCount[buffer] * _elementSize[buffer];
    // If requested we can invalidate the data in the specified range to hint at the driver to discard the old data
    if (invalidateRange) {
        glInvalidateBufferSubData(_bufferObjects[buffer], offset, dataCurrentSize);
    }

    if (!_bufferPersistent[buffer]) {
        // Update part of the data in the buffer at the specified buffer in the copy that's ready for writing
        glNamedBufferSubDataEXT(_bufferObjects[buffer], _startDestOffset[buffer] + offset, dataCurrentSize, data);
    } else {
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
        // Wait for the target part of the buffer to become available for writing
        _lockManager->WaitForLockedRange(_startDestOffset[buffer], bufferSize);
            // Offset the data pointer by the required offset taking in account the current data copy we are writing into
            U8* dst = (U8*)_bufferPersistentData[buffer] + _startDestOffset[buffer] + offset;
            // Update the data
            memcpy(dst, data, dataCurrentSize);
        // Lock the current buffer copy until uploading to GPU visible memory is finished
        _lockManager->LockRange(_startDestOffset[buffer], bufferSize);
    }
    // Update offset pointers for reading and writing
    _startDestOffset[buffer] = (_startDestOffset[buffer] + bufferSize) % (bufferSize * _sizeFactor[buffer]);
    _readOffset[buffer] = (_startDestOffset[buffer] + bufferSize) % (bufferSize * _sizeFactor[buffer]);
}

/// Update the appropriate attributes (either for drawing or for transform feedback)
void glGenericVertexData::SetAttributes(bool feedbackPass) {
    // Get the appropriate list of attributes
    attributeMap& map = feedbackPass ? _attributeMapFdbk : _attributeMapDraw;
    // And update them in turn
    for(attributeMap::value_type& it : map) {
        SetAttributeInternal(it.second);
    }
}

/// Update internal attribute data
void glGenericVertexData::SetAttributeInternal(AttributeDescriptor& descriptor) {
    DIVIDE_ASSERT(_elementSize[descriptor.bufferIndex()] != 0, "glGenericVertexData error: attribute's parent buffer has an invalid element size!");
    // Early out if the attribute didn't change
    if (!descriptor.dirty()) {
        return;
    }
    // If the attribute wasn't activate until now, enable it
    if (!descriptor.wasSet()) {
        glEnableVertexAttribArray(descriptor.attribIndex());
        descriptor.wasSet(true);
    }
    // Persistently mapped buffers are already bound when this function is called
    if (!_persistentMapped) {
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[descriptor.bufferIndex()]);
    }
    // Update the attribute data
    glVertexAttribPointer(descriptor.attribIndex(), 
                          descriptor.componentsPerElement(), 
                          GLUtil::GL_ENUM_TABLE::glDataFormat[descriptor.dataType()],
                          descriptor.normalized() ? GL_TRUE : GL_FALSE,
                          (GLsizei)descriptor.stride(),
                          (void*)(descriptor.offset() * _elementSize[descriptor.bufferIndex()]));

    if (descriptor.instanceDivisor() > 0) {
        glVertexAttribDivisor(descriptor.attribIndex(), descriptor.instanceDivisor());
    }
    // Inform the descriptor that the data was updated
    descriptor.clean();
}

/// Return the number of primitives written to a transform feedback buffer that used the specified query ID
U32 glGenericVertexData::GetFeedbackPrimitiveCount(U8 queryID) {
    DIVIDE_ASSERT(queryID < _numQueries && !_bufferObjects.empty(), "glGenericVertexData error: Current object isn't ready for query processing!");
    // Double buffered queries return the results from the previous draw call to avoid stalling the GPU pipeline
    U32 queryEntry = _doubleBufferedQuery ? _currentReadQuery : _currentWriteQuery;
    // If the requested query has valid data available, retrieve that data from the GPU
    if (_resultAvailable[queryEntry][queryID]){
        // Get the result of the previous query about the generated primitive count
        glGetQueryObjectuiv(_feedbackQueries[queryEntry][queryID], GL_QUERY_RESULT, &_prevResult[queryID]);
        // Mark the query entry data as invalid now
        _resultAvailable[queryEntry][queryID] = false;
    }
    // Return either the previous value or the current one depending on the previous check
    return _prevResult[queryID];
}

};