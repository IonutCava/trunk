#include "stdafx.h"

#include "Headers/glGenericBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"

namespace Divide {
glGenericBuffer::glGenericBuffer(GFXDevice& context, const BufferParams& params)
    : _elementCount(params._elementCount),
      _elementSize(params._elementSizeInBytes),
      _elementCountBindOffset(0),
      _ringSizeFactor(params._ringSizeFactor)
{
    size_t bufferSizeInBytes = _elementCount * _elementSize;
    size_t totalSizeInBytes = bufferSizeInBytes * _ringSizeFactor;
    
    BufferImplParams implParams;
    implParams._dataSizeInBytes = totalSizeInBytes;
    implParams._frequency = params._frequency;
    implParams._target = params._usage;
    implParams._name = params._name;
    implParams._initialData = params._data;

    _buffer = MemoryManager_NEW glBufferImpl(context, implParams);

    // Create sizeFactor copies of the data and store them in the buffer
    if (params._data != nullptr) {
        for (U8 i = 1; i < _ringSizeFactor; ++i) {
            _buffer->writeData(i * bufferSizeInBytes, bufferSizeInBytes, params._data);
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

void glGenericBuffer::writeData(GLuint elementCount,
                                GLuint elementOffset,
                                GLuint ringWriteOffset,
                                const bufferPtr data)
{
    // Calculate the size of the data that needs updating
    size_t dataCurrentSizeInBytes = elementCount * _elementSize;
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offsetInBytes = elementOffset * _elementSize;

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _elementSize * ringWriteOffset;
    }

    _buffer->writeData(offsetInBytes, dataCurrentSizeInBytes, data);
}

void glGenericBuffer::readData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset,
                               bufferPtr dataOut) 
{
    // Calculate the size of the data that needs updating
    size_t dataCurrentSizeInBytes = elementCount * _elementSize;
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offsetInBytes = elementOffset * _elementSize;

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _elementSize * ringReadOffset;
    }

    _buffer->readData(offsetInBytes, dataCurrentSizeInBytes, dataOut);
}

void glGenericBuffer::lockData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset)
{
    size_t rangeInBytes = elementCount * _elementSize;
    size_t offsetInBytes = elementOffset * _elementSize;

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _elementSize * ringReadOffset;
    }

    _buffer->lockRange(offsetInBytes, rangeInBytes);
}

GLintptr glGenericBuffer::getBindOffset(GLuint ringReadOffset)
{
    GLintptr retInBytes = static_cast<GLintptr>(_elementCountBindOffset * _elementSize);

    if (_ringSizeFactor > 1) {
        retInBytes += _elementCount * _elementSize * ringReadOffset;
    }

    return retInBytes;
}

void glGenericBuffer::setBindOffset(GLuint elementCountOffset) {
    _elementCountBindOffset = elementCountOffset;
}

}; //namespace Divide