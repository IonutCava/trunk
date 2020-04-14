#include "stdafx.h"

#include "Headers/glGenericBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"

namespace Divide {
glGenericBuffer::glGenericBuffer(GFXDevice& context, const BufferParams& params)
    : _elementCountBindOffset(0),
      _elementCount(params._elementCount),
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
    implParams._storageType = params._storageType;
    implParams._unsynced = params._unsynced;

    _buffer = MemoryManager_NEW glBufferImpl(context, implParams);

    // Create sizeFactor copies of the data and store them in the buffer
    if (params._data != nullptr) {
        for (U8 i = 1; i < _ringSizeFactor; ++i) {
            _buffer->writeData(i * bufferSizeInBytes, bufferSizeInBytes, (Byte*)params._data);
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

    _buffer->writeData(offsetInBytes, dataCurrentSizeInBytes, (Byte*)data);
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

    _buffer->readData(offsetInBytes, dataCurrentSizeInBytes, (Byte*)dataOut);
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

size_t glGenericBuffer::getBindOffset(GLuint ringReadOffset)
{
    size_t retInBytes = static_cast<size_t>(_elementCountBindOffset * _buffer->elementSize());

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



void glVertexDataContainer::rebuildCountAndIndexData(U32 drawCount, U32 indexCount, U32 firstIndex, size_t indexBufferSize) {
    if (_lastDrawCount == drawCount && _lastIndexCount == indexCount && _lastFirstIndex == firstIndex) {
        return;
    }

    if (_lastDrawCount != drawCount || _lastIndexCount != indexCount) {
        eastl::fill(eastl::begin(_countData), eastl::begin(_countData) + drawCount, indexCount);
    }

    if (indexBufferSize > 0 && (_lastDrawCount != drawCount || _lastFirstIndex != firstIndex)) {
        const U32 idxCountInternal = to_U32(drawCount * indexBufferSize);

        if (_indexOffsetData.size() < idxCountInternal) {
            _indexOffsetData.resize(idxCountInternal, firstIndex);
        }
        if (_lastFirstIndex != firstIndex) {
            eastl::fill(eastl::begin(_indexOffsetData), eastl::end(_indexOffsetData), firstIndex);
        }
    }
    _lastDrawCount = drawCount;
    _lastIndexCount = indexCount;
    _lastFirstIndex = firstIndex;
}

}; //namespace Divide