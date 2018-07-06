#include "Headers/glGenericVertexData.h"

glGenericVertexData::glGenericVertexData() : GenericVertexData(), _dataWriteActive(false)
{
}

glGenericVertexData::~glGenericVertexData()
{
   Clear();
}

void glGenericVertexData::Clear(){
    if(_numBuffers == 0)
        return;
    GLCheck(glDeleteVertexArrays(1, &_currentVAO));
    GLCheck(glDeleteBuffers(_numBuffers, &_bufferObjects[0]));
    _bufferObjects.clear();
    _currentVAO = 0;
}

void glGenericVertexData::Create(U8 numBuffers){
    Clear();

    _numBuffers = numBuffers;
    _bufferObjects.resize(numBuffers, 0);
    GLCheck(glGenVertexArrays(1, &_currentVAO));
    GLCheck(glGenBuffers(numBuffers, &_bufferObjects[0]));
}

 void glGenericVertexData::DrawInstanced(const PrimitiveType& type, U32 count, U32 min, U32 max){
    assert(_currentVAO != 0);
    assert(!_dataWriteActive);

    GL_API::setActiveVAO(_currentVAO);
    GLCheck(glDrawArraysInstanced(glPrimitiveTypeTable[type], min, max, count));
}

void glGenericVertexData::Draw(const PrimitiveType& type, U32 min, U32 max){
    assert(_currentVAO != 0);
    assert(!_dataWriteActive);

    GL_API::setActiveVAO(_currentVAO);
    GLCheck(glDrawArrays(glPrimitiveTypeTable[type], min, max));
}
