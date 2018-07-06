#include "Headers/glGenericVertexData.h"

glGenericVertexData::glGenericVertexData() : GenericVertexData(), _prevResult(nullptr)
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
}

glGenericVertexData::~glGenericVertexData()
{
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
}

void glGenericVertexData::Create(U8 numBuffers, U8 numQueries){
    assert(_bufferObjects.empty()); //< double create is not implemented yet as I don't quite need it. -Ionut
    _numQueries = numQueries;

    for (U8 i = 0; i < 2; ++i){
        _feedbackQueries[i] = New GLuint[_numQueries];
        _resultAvailable[i] = New bool[_numQueries];
    }

    _prevResult = New GLuint[_numQueries];
    for (U8 i = 0; i < 2; ++i){
        memset(_feedbackQueries[i], 0,     sizeof(GLuint) * _numQueries);
        memset(_resultAvailable[i], false, sizeof(bool)   * _numQueries);
    }

    memset(_prevResult, 0, sizeof(GLuint) * _numQueries);

    _bufferObjects.resize(numBuffers, 0);

    for (U8 i = 0; i < 2; ++i)
        glGenQueries(_numQueries, _feedbackQueries[i]);

    glGenVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
    glGenBuffers(numBuffers, &_bufferObjects[0]);
    glGenTransformFeedbacks(1, &_transformFeedback);
}

bool glGenericVertexData::frameStarted(const FrameEvent& evt) {
    if (!_bufferObjects.empty()){
        _currentWriteQuery = (_currentWriteQuery + 1) % 2;
        _currentReadQuery  = (_currentWriteQuery + 1) % 2;
    }

    return GenericVertexData::frameStarted(evt);
}

void glGenericVertexData::DrawInternal(const PrimitiveType& type, U32 min, U32 max, U32 instanceCount, U8 queryID, bool drawToBuffer) {
    if (instanceCount == 0) return;

    bool feedbackActive = (drawToBuffer && !_feedbackBuffers.empty());

    GL_API::setActiveVAO(_vertexArray[feedbackActive ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);

    if (feedbackActive){
       glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _transformFeedback);
       glBeginTransformFeedback(glPrimitiveTypeTable[type]);
       glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, _feedbackQueries[_currentWriteQuery][queryID]);
    } 
    
    glDrawArraysInstanced(glPrimitiveTypeTable[type], min, max, instanceCount);

    GL_API::registerDrawCall();

    if (feedbackActive) {
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glEndTransformFeedback();
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        _resultAvailable[_currentWriteQuery][queryID] = true;
    }
}

void glGenericVertexData::SetBuffer(U32 buffer, size_t dataSize, void* data, bool dynamic, bool stream) {
    assert(buffer >= 0 && buffer < _bufferObjects.size());

    GLenum flag = isFeedbackBuffer(buffer) ? (dynamic ? (stream ? GL_STREAM_COPY : GL_DYNAMIC_COPY) : GL_STATIC_COPY):
                                             (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);
                                             

    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, flag);
}

void glGenericVertexData::UpdateBuffer(U32 buffer, size_t dataSize, void* data, U32 offset, size_t currentSize, bool dynamic, bool stream) {
    SetBuffer(buffer, dataSize, nullptr, dynamic, stream);
    glBufferSubData(GL_ARRAY_BUFFER, offset, currentSize, data);
}

void glGenericVertexData::UpdateAttribute(U32 index, U32 buffer, U32 divisor, GLsizei size, GLboolean normalized, U32 stride, GLvoid* offset, GLenum type, bool feedbackAttrib) {
    GL_API::setActiveVAO(_vertexArray[feedbackAttrib ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);
    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _bufferObjects[buffer]);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
    if (divisor > 0) glVertexAttribDivisor(index, divisor);
}

U32 glGenericVertexData::GetFeedbackPrimitiveCount(U8 queryID) {
    assert(queryID < _numQueries && !_bufferObjects.empty());
    if (_resultAvailable[_currentReadQuery][queryID]){
        // get the result of the previous query about the generated primitive count
        glGetQueryObjectuiv(_feedbackQueries[_currentReadQuery][queryID], GL_QUERY_RESULT, &_prevResult[queryID]);
        _resultAvailable[_currentReadQuery][queryID] = false;
    }
    return _prevResult[queryID];
}