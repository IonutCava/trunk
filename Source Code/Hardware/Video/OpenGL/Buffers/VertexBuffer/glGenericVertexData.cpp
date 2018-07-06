#include "Headers/glGenericVertexData.h"

glGenericVertexData::glGenericVertexData() : GenericVertexData(), _feedbackQueries(nullptr), _resultAvailable(nullptr), _prevResult(nullptr)
{
    _vertexArray[GVD_USAGE_DRAW] = _vertexArray[GVD_USAGE_FDBCK] = 0;
    _transformFeedback = 0;
    _numQueries = 0;
}

glGenericVertexData::~glGenericVertexData()
{
    if (!_bufferObjects.empty()){
        glDeleteVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
        glDeleteBuffers((GLsizei)_bufferObjects.size(), &_bufferObjects[0]);
        _vertexArray[GVD_USAGE_DRAW] = _vertexArray[GVD_USAGE_FDBCK] = 0;
    }
    glDeleteTransformFeedbacks(1, &_transformFeedback);
    glDeleteQueries(_numQueries, _feedbackQueries);
    _transformFeedback = 0;
    _bufferObjects.clear();
    _feedbackBuffers.clear();
    _feedbackBindPoints.clear();
    SAFE_DELETE_ARRAY(_feedbackQueries);
    SAFE_DELETE_ARRAY(_resultAvailable);
    SAFE_DELETE_ARRAY(_prevResult);
}

void glGenericVertexData::Create(U8 numBuffers, U8 numQueries){
    assert(_bufferObjects.empty()); //< double create is not implemented yet as I don't quite need it. -Ionut
    _numQueries = numQueries;
    _feedbackQueries = New GLuint[_numQueries];
    _resultAvailable = New bool[_numQueries];
    _prevResult      = New GLint[_numQueries];

    memset(_feedbackQueries, 0,     sizeof(GLuint) * _numQueries);
    memset(_resultAvailable, false, sizeof(bool) * _numQueries);
    memset(_prevResult,      0,     sizeof(GLint)* _numQueries);
    _bufferObjects.resize(numBuffers, 0);
    glGenQueries(_numQueries, _feedbackQueries);
    glGenVertexArrays(GVD_USAGE_PLACEHOLDER, &_vertexArray[0]);
    glGenBuffers(numBuffers, &_bufferObjects[0]);
    glGenTransformFeedbacks(1, &_transformFeedback);
}

void glGenericVertexData::DrawInternal(const PrimitiveType& type, U32 min, U32 max, U32 instanceCount, U8 queryID, bool drawToBuffer) {

    bool feedbackActive = (drawToBuffer && !_feedbackBuffers.empty());

    GL_API::setActiveVAO(_vertexArray[feedbackActive ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);

    if (feedbackActive){
       glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _transformFeedback);
       glBeginTransformFeedback(glPrimitiveTypeTable[type]);
       glBeginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, queryID, _feedbackQueries[queryID]);
    } 
    
    glDrawArraysInstanced(glPrimitiveTypeTable[type], min, max, std::max(instanceCount, (GLuint)1));

    GL_API::registerDrawCall();

    if (feedbackActive) {
        glEndQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, queryID);
        glEndTransformFeedback();
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        _resultAvailable[queryID] = true;
    }
}

void glGenericVertexData::SetBuffer(U32 buffer, size_t dataSize, void* data, bool dynamic, bool stream) {
    assert(buffer >= 0 && buffer < _bufferObjects.size());

    GLenum flag = isFeedbackBuffer(buffer) ? (dynamic ? (stream ? GL_STREAM_COPY : GL_DYNAMIC_COPY) : GL_STATIC_COPY):
                                             (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);
                                             

    GL_API::setActiveVB(_bufferObjects[buffer]);
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, flag);
}

void glGenericVertexData::UpdateBuffer(U32 buffer, size_t dataSize, void* data, U32 offset, size_t currentSize, bool dynamic, bool stream) {
    SetBuffer(buffer, dataSize, nullptr, dynamic, stream);
    glBufferSubData(GL_ARRAY_BUFFER, offset, currentSize, data);
}

void glGenericVertexData::UpdateAttribute(U32 index, U32 buffer, U32 divisor, GLsizei size, GLboolean normalized, U32 stride, GLvoid* offset, GLenum type, bool feedbackAttrib) {
    GL_API::setActiveVAO(_vertexArray[feedbackAttrib ? GVD_USAGE_FDBCK : GVD_USAGE_DRAW]);
    GL_API::setActiveVB(_bufferObjects[buffer]);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, offset);
    if (divisor > 0) glVertexAttribDivisor(index, divisor);
}

I32 glGenericVertexData::GetFeedbackPrimitiveCount(U8 queryID) {
    assert(queryID < _numQueries);

    if (_resultAvailable[queryID]){
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _transformFeedback);
        // get the result of the previous query about the generated primitive count
        glGetQueryIndexediv(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, queryID, GL_CURRENT_QUERY, &_prevResult[queryID]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        _resultAvailable[queryID] = false;
    }
    return _prevResult[queryID];
}