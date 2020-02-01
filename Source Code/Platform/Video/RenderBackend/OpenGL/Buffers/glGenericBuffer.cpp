#include "stdafx.h"

#include "Headers/glGenericBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"

namespace Divide {
glGenericBuffer::glGenericBuffer(GFXDevice& context, const BufferParams& params)
    : _elementCount(params._elementCount),
      _elementCountBindOffset(0),
      _ringSizeFactor(params._ringSizeFactor)
{
    const size_t bufferSizeInBytes = _elementCount * params._elementSizeInBytes;
    const size_t totalSizeInBytes = bufferSizeInBytes * _ringSizeFactor;
    
    BufferImplParams implParams = {};
    implParams._dataSize = totalSizeInBytes;
    implParams._elementSize = params._elementSizeInBytes;
    implParams._frequency = params._frequency;
    implParams._target = params._usage;
    implParams._name = params._name;
    implParams._initialData = params._data;
    implParams._zeroMem = params._zeroMem;
    implParams._storageType = params._storageType;
    implParams._unsynced = params._unsynced;

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
    const size_t dataCurrentSizeInBytes = elementCount * _buffer->elementSize();
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offsetInBytes = elementOffset * _buffer->elementSize();

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _buffer->elementSize() * ringWriteOffset;
    }

    _buffer->writeData(offsetInBytes, dataCurrentSizeInBytes, data);
}

void glGenericBuffer::readData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset,
                               bufferPtr dataOut) 
{
    // Calculate the size of the data that needs updating
    const size_t dataCurrentSizeInBytes = elementCount * _buffer->elementSize();
    // Calculate the offset in the buffer in bytes from which to start writing
    size_t offsetInBytes = elementOffset * _buffer->elementSize();

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _buffer->elementSize() * ringReadOffset;
    }

    _buffer->readData(offsetInBytes, dataCurrentSizeInBytes, dataOut);
}

void glGenericBuffer::lockData(GLuint elementCount,
                               GLuint elementOffset,
                               GLuint ringReadOffset,
                               bool flush)
{
    const size_t rangeInBytes = elementCount * _buffer->elementSize();
    size_t offsetInBytes = elementOffset * _buffer->elementSize();

    if (_ringSizeFactor > 1) {
        offsetInBytes += _elementCount * _buffer->elementSize() * ringReadOffset;
    }

    _buffer->lockRange(offsetInBytes, rangeInBytes, flush);
}

void glGenericBuffer::clearData(GLuint elementOffset, GLuint ringWriteOffset)
{
    const size_t rangeInBytes = _elementCount * _buffer->elementSize();
    size_t offsetInBytes = elementOffset * _buffer->elementSize();

    if (_ringSizeFactor > 1) {
        offsetInBytes += rangeInBytes * ringWriteOffset;
    }

    _buffer->zeroMem(offsetInBytes, rangeInBytes);
}

GLintptr glGenericBuffer::getBindOffset(GLuint ringReadOffset)
{
    GLintptr retInBytes = static_cast<GLintptr>(_elementCountBindOffset * _buffer->elementSize());

    if (_ringSizeFactor > 1) {
        retInBytes += _elementCount * _buffer->elementSize() * ringReadOffset;
    }

    return retInBytes;
}

void glGenericBuffer::setBindOffset(GLuint elementCountOffset) noexcept {
    _elementCountBindOffset = elementCountOffset;
}

glBufferImpl* glGenericBuffer::bufferImpl() const noexcept {
    return _buffer;
}

}; //namespace Divide