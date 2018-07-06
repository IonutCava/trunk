#include "Headers/glGenericVertexData.h"

glGenericVertexData::glGenericVertexData(bool persistentMapped) : GenericVertexData(persistentMapped), _prevResult(nullptr),
                                                                                                       _bufferSet(nullptr),
                                                                                                       _bufferPersistent(nullptr),
                                                                                                       _bufferPersistentData(nullptr),
                                                                                                       _elementCount(nullptr),
                                                                                                       _elementSize(nullptr),
                                                                                                       _startDestOffset(nullptr),
                                                                                                       _readOffset(nullptr)
{
    _vertexArray[GVD_USAGE_DRAW] = _vertexArray[GVD_USAGE_FDBCK] = 0;
    _transformFeedback = 0;
    _numQueries  = 0;
    _currentWriteQuery = 0;
    _currentReadQuery = 0;
    for (U8 i = 0; i < 2; ++i){
        _feedbackQueries[i] = nullptr;
        _resultAvailable[i] = nullptr;
    }

    _lockManager = persistentMapped ? New glBufferLockManager(true) : nullptr;
}

glGenericVertexData::~glGenericVertexData()
{
    for (U8 i = 0; i < _bufferObjects.size(); ++i){
        if (_bufferPersistent[i]){
            glBindBuffer(GL_ARRAY_BUFFER, _bufferObjects[i]);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
    }

    if (!_bufferObjects.empty()){
        glDeleteVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
        glDeleteBuffers((GLsizei)_bufferObjects.size(), &_bufferObjects[0]);
        _vertexArray[GVD_USAGE_DRAW] = _vertexArray[GVD_USAGE_FDBCK] = 0;
    }
    glDeleteTransformFeedbacks(1, &_transformFeedback);

    for (U8 i = 0; i < 2; ++i)
        glDeleteQueries(_numQueries, _feedbackQueries[i]);

    _transformFeedback = 0;
    _bufferObjects.clear();
    _feedbackBuffers.clear();

    for (U8 i = 0; i < 2; ++i){
        SAFE_DELETE_ARRAY(_feedbackQueries[i]);
        SAFE_DELETE_ARRAY(_resultAvailable[i]);
    }
    SAFE_DELETE_ARRAY(_prevResult);
    SAFE_DELETE_ARRAY(_bufferSet);
    SAFE_DELETE_ARRAY(_bufferPersistent);
    SAFE_DELETE_ARRAY(_elementCount);
    SAFE_DELETE_ARRAY(_elementSize);
    SAFE_DELETE_ARRAY(_startDestOffset);
    SAFE_DELETE_ARRAY(_readOffset);
    free(_bufferPersistentData);
    
    SAFE_DELETE(_lockManager);
}

void glGenericVertexData::Create(U8 numBuffers, U8 numQueries){
    assert(_bufferObjects.empty()); //< double create is not implemented yet as I don't quite need it. -Ionut
    glGenVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
    glGenTransformFeedbacks(1, &_transformFeedback);

    _bufferObjects.resize(numBuffers, 0);
    glGenBuffers(numBuffers, &_bufferObjects[0]);

    _numQueries = numQueries;
    for (U8 i = 0; i < 2; ++i){
        _feedbackQueries[i] = New GLuint[_numQueries];
        _resultAvailable[i] = New bool[_numQueries];
    }

    for (U8 i = 0; i < 2; ++i){
        memset(_feedbackQueries[i], 0, sizeof(GLuint) * _numQueries);
        memset(_resultAvailable[i], false, sizeof(bool)   * _numQueries);
        glGenQueries(_numQueries, _feedbackQueries[i]);
    }

    _prevResult = New GLuint[_numQueries];
    memset(_prevResult, 0, sizeof(GLuint) * _numQueries);

    _bufferSet = new bool[numBuffers];
    memset(_bufferSet, false, numBuffers * sizeof(bool));

    _elementCount = new GLuint[numBuffers];
    memset(_elementCount, 0, numBuffers * sizeof(GLuint));

    _elementSize = new size_t[numBuffers];
    memset(_elementSize, 0, numBuffers * sizeof(size_t));
    
    _startDestOffset = new size_t[numBuffers];
    memset(_startDestOffset, 0, numBuffers * sizeof(size_t));

    _readOffset = new size_t[numBuffers];
    memset(_readOffset, 0, numBuffers * sizeof(size_t));

    _bufferPersistent = new bool[numBuffers];
    memset(_bufferPersistent, false, numBuffers * sizeof(bool));

    _bufferPersistentData = (GLvoid**)malloc(sizeof(GLvoid*) * numBuffers);

}

bool glGenericVertexData::frameStarted(const FrameEvent& evt) {
    if (!_bufferObjects.empty() && _doubleBufferedQuery){
        _currentWriteQuery = (_currentWriteQuery + 1) % 2;
        _currentReadQuery  = (_currentWriteQuery + 1) % 2;
    }

    return GenericVertexData::frameStarted(evt);
}

void glGenericVertexData::BindFeedbackBufferRange(U32 buffer, size_t elementCountOffset, size_t elementCount){
    assert(isFeedbackBuffer(buffer));
    GL_API::setActiveTransformFeedback(_transformFeedback);
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, getBindPoint(_bufferObjects[buffer]), _bufferObjects[buffer], elementCountOffset * _elementSize[buffer], elementCount * _elementSize[buffer]);
}

void glGenericVertexData::DrawInternal(const PrimitiveType& type, U32 min, U32 max, U32 instanceCount, U8 queryID, bool drawToBuffer) {
    if (instanceCount == 0) return;

    bool feedbackActive = (drawToBuffer && !_feedbackBuffers.empty());

    GL_API::setActiveVAO(_vertexArray[feedbackActive ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);

    SetAttributes(feedbackActive);

    if (feedbackActive){
       GL_API::setActiveTransformFeedback(_transformFeedback);
       glBeginTransformFeedback(glPrimitiveTypeTable[type]);
       glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _feedbackQueries[_currentWriteQuery][queryID]);
    } 
    
    glDrawArraysInstanced(glPrimitiveTypeTable[type], min, max, instanceCount);

    GL_API::registerDrawCall();

    if (feedbackActive) {
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();
        _resultAvailable[_currentWriteQuery][queryID] = true;
    }
}

void glGenericVertexData::SetBuffer(U32 buffer, U32 elementCount, size_t elementSize, void* data, bool dynamic, bool stream, bool persistentMapped) {
    assert(buffer >= 0 && buffer < _bufferObjects.size());
    // glBufferData on persistentMapped buffers is not allowed
    assert(!(_bufferSet[buffer] && _persistentMapped && persistentMapped));
    assert((persistentMapped && _persistentMapped) || !persistentMapped);

    _elementCount[buffer] = elementCount;
    _elementSize[buffer] = elementSize;
    size_t bufferSize = elementCount * elementSize;

    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
    if (persistentMapped){
        GLenum flag = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize * 3, NULL, flag);

        _bufferPersistentData[buffer] = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize * 3, flag);
        if (data != nullptr){
            void* dst = nullptr;
            _lockManager->WaitForLockedRange(0, bufferSize * 3);
                dst = (unsigned char*)_bufferPersistentData[buffer] + bufferSize * 0;
                memcpy(dst, data, bufferSize);
                dst = (unsigned char*)_bufferPersistentData[buffer] + bufferSize * 1;
                memcpy(dst, data, bufferSize);
                dst = (unsigned char*)_bufferPersistentData[buffer] + bufferSize * 2;
                memcpy(dst, data, bufferSize);
            _lockManager->WaitForLockedRange(0, bufferSize * 3);
        }
    } else{
        GLenum flag = isFeedbackBuffer(buffer) ? (dynamic ? (stream ? GL_STREAM_COPY : GL_DYNAMIC_COPY) : GL_STATIC_COPY) :
                                                 (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);

        glBufferData(GL_ARRAY_BUFFER, _elementCount[buffer] * elementSize, data, flag);
    }
    _bufferSet[buffer] = true;
    _bufferPersistent[buffer] = persistentMapped;
}

void glGenericVertexData::UpdateBuffer(U32 buffer, U32 elementCount, void* data, U32 offset, bool dynamic, bool stream) {
    size_t dataCurrentSize = elementCount * _elementSize[buffer];
    if (!_bufferPersistent[buffer]){
        SetBuffer(buffer, _elementCount[buffer], _elementSize[buffer], nullptr, dynamic, stream, false);
        glBufferSubData(GL_ARRAY_BUFFER, offset, dataCurrentSize, data);
    }else{
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
        size_t bufferSize = _elementCount[buffer] * _elementSize[buffer];

        _lockManager->WaitForLockedRange(_startDestOffset[buffer], bufferSize);

        void* dst = (unsigned char*)_bufferPersistentData[buffer] + _startDestOffset[buffer] + offset;
         memcpy(dst, data, dataCurrentSize);

        // Lock this area for the future.
        _lockManager->LockRange(_startDestOffset[buffer], bufferSize);
            
        _startDestOffset[buffer] = (_startDestOffset[buffer] + bufferSize) % (bufferSize * 3);
        _readOffset[buffer] = (_startDestOffset[buffer] + bufferSize) % (bufferSize * 3);
    }
}

void glGenericVertexData::SetAttributeInternal(AttributeDescriptor& descriptor){
    assert(_elementSize[descriptor._parentBuffer] != 0);
    if(!descriptor._dirty) return;

    if(!descriptor._wasSet){
        glEnableVertexAttribArray(descriptor._index);
        descriptor._wasSet = true;
    }
    if (!_persistentMapped){
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[descriptor._parentBuffer]);
    }
    glVertexAttribPointer(descriptor._index, descriptor._componentsPerElement, glDataFormat[descriptor._type], descriptor._normalized ? GL_TRUE : GL_FALSE,
                          (GLsizei)descriptor._stride, (GLvoid*)(descriptor._elementCountOffset * _elementSize[descriptor._parentBuffer]));
    if (descriptor._divisor > 0) glVertexAttribDivisor(descriptor._index, descriptor._divisor);

    descriptor._dirty = false;
}

void glGenericVertexData::SetAttributes(bool feedbackPass) {
    attributeMap& map = feedbackPass ? _attributeMapFdbk : _attributeMapDraw;
    FOR_EACH(attributeMap::value_type& it, map){
        SetAttributeInternal(it.second);
    }
}

U32 glGenericVertexData::GetFeedbackPrimitiveCount(U8 queryID) {
    assert(queryID < _numQueries && !_bufferObjects.empty());
    U32 queryEntry = _doubleBufferedQuery ? _currentReadQuery : _currentWriteQuery;
    if (_resultAvailable[queryEntry][queryID]){
        // get the result of the previous query about the generated primitive count
        glGetQueryObjectuiv(_feedbackQueries[queryEntry][queryID], GL_QUERY_RESULT, &_prevResult[queryID]);
        _resultAvailable[queryEntry][queryID] = false;
    }

    return _prevResult[queryID];
}