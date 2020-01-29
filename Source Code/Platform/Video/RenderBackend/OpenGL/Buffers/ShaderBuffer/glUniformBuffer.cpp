#include "stdafx.h"

#include "Headers/glUniformBuffer.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glGenericBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferLockManager.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

#include <iomanip>

namespace Divide {

glUniformBuffer::glUniformBuffer(GFXDevice& context,
                                 const ShaderBufferDescriptor& descriptor)
    : ShaderBuffer(context, descriptor)
{
    _maxSize = (_usage != Usage::CONSTANT_BUFFER) ? GL_API::s_SSBMaxSize : GL_API::s_UBMaxSize;

    _allignedBufferSize = realign_offset(_bufferSize, alignmentRequirement(_usage));

    BufferImplParams implParams = {};
    implParams._dataSize = _allignedBufferSize * queueLength();
    implParams._elementSize = _elementSize;
    implParams._frequency = _frequency;
    implParams._initialData = descriptor._initialData;
    implParams._name = _name.empty() ? nullptr : _name.c_str();
    implParams._zeroMem = descriptor._initialData == nullptr;
    implParams._explicitFlush = !BitCompare(_flags, ShaderBuffer::Flags::AUTO_RANGE_FLUSH);

    implParams._target = _usage == Usage::UNBOUND_BUFFER 
                                 ? GL_SHADER_STORAGE_BUFFER
                                 : _usage == Usage::CONSTANT_BUFFER
                                           ? GL_UNIFORM_BUFFER
                                           : GL_ATOMIC_COUNTER_BUFFER;

    implParams._storageType = BitCompare(_flags, ShaderBuffer::Flags::ALLOW_THREADED_WRITES) || _usage == Usage::ATOMIC_COUNTER
                                ? BufferStorageType::IMMUTABLE
                                : BufferStorageType::AUTO;

    implParams._unsynced =  implParams._storageType != BufferStorageType::IMMUTABLE || 
                            BitCompare(_flags, ShaderBuffer::Flags::NO_SYNC) ||
                            _frequency == BufferUpdateFrequency::ONCE;

    _buffer = MemoryManager_NEW glBufferImpl(context, implParams);
}

glUniformBuffer::~glUniformBuffer() 
{
    MemoryManager::DELETE(_buffer);
}

GLuint glUniformBuffer::bufferID() const {
    return _buffer->bufferID();
}

void glUniformBuffer::clearData(U32 offsetElementCount,
                                U32 rangeElementCount) {
    OPTICK_EVENT();

    if (rangeElementCount > 0) {
        GLsizeiptr rangeInBytes = rangeElementCount * _elementSize;
        GLintptr offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= _allignedBufferSize &&
            "glUniformBuffer::UpdateData error: was called with an "
            "invalid range (buffer overflow)!");

        if (queueLength() > 1) {
            offsetInBytes += queueWriteIndex() * _allignedBufferSize;
        }

        size_t req = alignmentRequirement(_usage);
        if (offsetInBytes % req != 0) {
            offsetInBytes = (offsetInBytes + req - 1) / req * req;
        }

        _buffer->zeroMem(offsetInBytes, rangeInBytes);
    }
}

void glUniformBuffer::readData(U32 offsetElementCount,
                               U32 rangeElementCount,
                               bufferPtr result) const {

    if (rangeElementCount > 0) {
        GLsizeiptr rangeInBytes = rangeElementCount * _elementSize;
        GLintptr offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= _allignedBufferSize &&
            "glUniformBuffer::UpdateData error: was called with an "
            "invalid range (buffer overflow)!");

        if (queueLength() > 1) {
            offsetInBytes += queueReadIndex() * _allignedBufferSize;
        }

        size_t req = alignmentRequirement(_usage);
        if (offsetInBytes % req != 0) {
            offsetInBytes = (offsetInBytes + req - 1) / req * req;
        }

        _buffer->readData(offsetInBytes, rangeInBytes, result);
    }
}

void glUniformBuffer::writeData(U32 offsetElementCount,
                                U32 rangeElementCount,
                                const bufferPtr data) {

    writeBytes(static_cast<GLintptr>(offsetElementCount * _elementSize),
               static_cast<GLsizeiptr>(rangeElementCount * _elementSize),
               data);
}

void glUniformBuffer::writeBytes(ptrdiff_t offsetInBytes,
                                 ptrdiff_t rangeInBytes,
                                 const bufferPtr data) {

    if (rangeInBytes == 0) {
        return;
    }

    if (rangeInBytes == static_cast<ptrdiff_t>(_elementCount * _elementSize)) {
        rangeInBytes = _allignedBufferSize;
    }
    DIVIDE_ASSERT(offsetInBytes + rangeInBytes <= (ptrdiff_t)_allignedBufferSize,
        "glUniformBuffer::UpdateData error: was called with an "
        "invalid range (buffer overflow)!");

    if (queueLength() > 1) {
        offsetInBytes += queueWriteIndex() * _allignedBufferSize;
    }

    const size_t req = alignmentRequirement(_usage);
    if (offsetInBytes % req != 0) {
        offsetInBytes = (offsetInBytes + req - 1) / req * req;
    }

    _buffer->writeData(offsetInBytes, rangeInBytes, data);
}

bool glUniformBuffer::bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) {
    BufferWriteData data = {};
    bool ret = bindRange(bindIndex, offsetElementCount, rangeElementCount, data);
    bufferImpl()->lockRange(data._offset, data._range, data._flush);
    return ret;
}

bool glUniformBuffer::bindRange(U8 bindIndex,
                                U32 offsetElementCount,
                                U32 rangeElementCount,
                                BufferWriteData& dataOut) {
    if (rangeElementCount == 0) {
        rangeElementCount = _elementCount;
    }

    dataOut._handle = bufferImpl()->bufferID();
    dataOut._range = to_size(rangeElementCount * _elementSize);
    dataOut._offset = to_size(offsetElementCount * _elementSize);
    if (queueLength() > 1) {
        dataOut._offset += to_size(queueReadIndex() * _allignedBufferSize);
    }
    size_t req = alignmentRequirement(_usage);
    if (dataOut._offset % req != 0) {
        dataOut._offset = (dataOut._offset + req - 1) / req * req;
    }
    dataOut._flush = BitCompare(_flags, ShaderBuffer::Flags::ALLOW_THREADED_WRITES);

    assert(static_cast<size_t>(dataOut._range) <= _maxSize &&
           "glUniformBuffer::bindRange: attempted to bind a larger shader block than is allowed on the current platform");

    assert(dataOut._offset % req == 0);
    return _buffer->bindRange(bindIndex, dataOut._offset, dataOut._range);
}

bool glUniformBuffer::bind(U8 bindIndex) {
    return bindRange(bindIndex, 0, _elementCount);
}

void glUniformBuffer::onGLInit() {
    ShaderBuffer::s_boundAlignmentRequirement = GL_API::s_UBOffsetAlignment;
    ShaderBuffer::s_unboundAlignmentRequirement = GL_API::s_SSBOffsetAlignment;
}

glBufferImpl* glUniformBuffer::bufferImpl() const {
    return _buffer;
}

};  // namespace Divide
