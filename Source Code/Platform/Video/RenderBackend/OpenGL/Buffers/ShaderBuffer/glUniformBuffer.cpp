#include "stdafx.h"

#include "Headers/glUniformBuffer.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferImpl.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glGenericBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/Headers/glBufferLockManager.h"

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
    implParams._updateUsage = descriptor._updateUsage;
    implParams._explicitFlush = !BitCompare(_flags, ShaderBuffer::Flags::AUTO_RANGE_FLUSH);

    implParams._target = _usage == Usage::UNBOUND_BUFFER 
                                 ? GL_SHADER_STORAGE_BUFFER
                                 : _usage == Usage::CONSTANT_BUFFER
                                           ? GL_UNIFORM_BUFFER
                                           : GL_ATOMIC_COUNTER_BUFFER;

    if (BitCompare(_flags, ShaderBuffer::Flags::IMMUTABLE_STORAGE) || 
        BitCompare(_flags, ShaderBuffer::Flags::ALLOW_THREADED_WRITES) ||
        _usage == Usage::ATOMIC_COUNTER) 
    {
        implParams._storageType = BufferStorageType::IMMUTABLE;
    } else if (BitCompare(_flags, ShaderBuffer::Flags::AUTO_STORAGE)) {
        implParams._storageType = BufferStorageType::AUTO;
    } else {
        implParams._storageType = BufferStorageType::NORMAL;
    }

    implParams._unsynced =  (implParams._storageType != BufferStorageType::IMMUTABLE &&
                             implParams._storageType != BufferStorageType::AUTO) ||
                            BitCompare(_flags, ShaderBuffer::Flags::NO_SYNC) ||
                            _frequency == BufferUpdateFrequency::ONCE;

    _buffer = MemoryManager_NEW glBufferImpl(context, implParams);
}

glUniformBuffer::~glUniformBuffer() 
{
    MemoryManager::DELETE(_buffer);
}

void glUniformBuffer::clearData(U32 offsetElementCount,
                                U32 rangeElementCount) {
    OPTICK_EVENT();

    if (rangeElementCount > 0) {
        const size_t rangeInBytes = rangeElementCount * _elementSize;
        size_t offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= _allignedBufferSize &&
            "glUniformBuffer::UpdateData error: was called with an "
            "invalid range (buffer overflow)!");

        if (queueLength() > 1) {
            offsetInBytes += queueWriteIndex() * _allignedBufferSize;
        }

        const size_t req = alignmentRequirement(_usage);
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
        const size_t rangeInBytes = rangeElementCount * _elementSize;
        size_t offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= _allignedBufferSize &&
            "glUniformBuffer::UpdateData error: was called with an "
            "invalid range (buffer overflow)!");

        if (queueLength() > 1) {
            offsetInBytes += queueReadIndex() * _allignedBufferSize;
        }

        const size_t req = alignmentRequirement(_usage);
        if (offsetInBytes % req != 0) {
            offsetInBytes = (offsetInBytes + req - 1) / req * req;
        }

        _buffer->readData(offsetInBytes, rangeInBytes, (Byte*)result);
    }
}

void glUniformBuffer::writeData(U32 offsetElementCount,
                                U32 rangeElementCount,
                                const bufferPtr data) {

    writeBytes(static_cast<ptrdiff_t>(offsetElementCount * _elementSize),
               static_cast<ptrdiff_t>(rangeElementCount * _elementSize),
               data);
}

void glUniformBuffer::writeBytes(ptrdiff_t offsetInBytes,
                                 ptrdiff_t rangeInBytes,
                                 const bufferPtr data) {

    OPTICK_EVENT();

    if (rangeInBytes == 0) {
        return;
    }

    const ptrdiff_t writeRange = rangeInBytes;
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

    bufferImpl()->writeData(offsetInBytes, writeRange, reinterpret_cast<Byte*>(data));
}

bool glUniformBuffer::bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) {
    BufferLockEntry data = {};
    data._buffer = bufferImpl();
    data._flush = BitCompare(_flags, ShaderBuffer::Flags::ALLOW_THREADED_WRITES);

    if (rangeElementCount == 0) {
        rangeElementCount = _elementCount;
    }

    data._length = to_size(rangeElementCount * _elementSize);
    data._offset = to_size(offsetElementCount * _elementSize);
    if (queueLength() > 1) {
        data._offset += to_size(queueReadIndex() * _allignedBufferSize);
    }
    const size_t req = alignmentRequirement(_usage);
    if (data._offset % req != 0) {
        data._offset = (data._offset + req - 1) / req * req;
    }
    assert(data._offset % req == 0);
    assert(static_cast<size_t>(data._length) <= _maxSize &&
           "glUniformBuffer::bindRange: attempted to bind a larger shader block than is allowed on the current platform");

    const bool wasBound = data._buffer->bindRange(bindIndex, data._offset, data._length);
    GL_API::registerBufferBind(std::move(data));

    return wasBound;
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
