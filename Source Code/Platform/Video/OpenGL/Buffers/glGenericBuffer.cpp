#include "Headers/glGenericBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"

namespace Divide {
glGenericBuffer::glGenericBuffer(const BufferParams& params)
    : _elementCount(params._elementCount),
      _elementSize(params._elementSizeInBytes),
      _elementCountBindOffset(0),
      _ringSizeFactor(params._ringSizeFactor)
{
    size_t bufferSize = _elementCount * _elementSize;
    size_t totalSize = bufferSize * _ringSizeFactor;
    
    BufferImplParams implParams;
    implParams._dataSizeInBytes = totalSize;
    implParams._frequency = params._frequency;
    implParams._target = params._usage;
    implParams._name = params._name;
    if (_ringSizeFactor == 1) {
        implParams._initialData = params._data;
    }

    _buffer = MemoryManager_NEW glBufferImpl(implParams);

    // Create sizeFactor copies of the data and store them in the buffer
    if (params._data != nullptr && _ringSizeFactor > 1) {
        for (U8 i = 0; i < _ringSizeFactor; ++i) {
            _buffer->updateData(i * bufferSize, bufferSize, params._data);
        }
    }
}

glGenericBuffer::~glGenericBuffer()
{
    MemoryManager::DELETE(_buffer);
}

GLuint glGenericBuffer::bufferHandle() const {
    return _buffer->bufferID();
}

void glGenericBuffer::updateData(GLuint elementCount,
                                 GLuint elementOffset,
                                 GLuint ringWriteOffset,
                                 const bufferPtr data)
{
    // Calculate the size of the data that needs updating
    size_t dataCurrentSize = elementCount * _elementSize;
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offset = elementOffset * _elementSize;

    if (_ringSizeFactor > 1) {
        offset += _elementCount * _elementSize * ringWriteOffset;
    }

    _buffer->updateData(offset, dataCurrentSize, data);
}

void glGenericBuffer::readData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset,
                               bufferPtr dataOut) 
{
    // Calculate the size of the data that needs updating
    size_t dataCurrentSize = elementCount * _elementSize;
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offset = elementOffset * _elementSize;

    if (_ringSizeFactor > 1) {
        offset += _elementCount * _elementSize * ringReadOffset;
    }

    _buffer->readData(offset, dataCurrentSize, dataOut);
}

void glGenericBuffer::lockData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset)
{
    size_t range = elementCount * _elementSize;
    size_t offset = 0;

    if (_ringSizeFactor > 1) {
        offset += _elementCount * _elementSize * ringReadOffset;
    }

    _buffer->lockRange(offset, range);
}

GLintptr glGenericBuffer::getBindOffset(GLuint ringReadOffset)
{
    GLintptr ret = static_cast<GLintptr>(_elementCountBindOffset * _elementSize);

    if (_ringSizeFactor > 1) {
        ret += _elementCount * _elementSize * ringReadOffset;
    }

    return ret;
}

void glGenericBuffer::setBindOffset(GLuint elementCountOffset) {
    _elementCountBindOffset = elementCountOffset;
}

}; //namespace Divide