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

void glUniformBuffer::clearData(ptrdiff_t offsetElementCount,
                                ptrdiff_t rangeElementCount) {
    if (rangeElementCount > 0) {
        ptrdiff_t rangeInBytes = rangeElementCount * _elementSize;
        ptrdiff_t offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= (ptrdiff_t)_allignedBufferSize &&
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

void glUniformBuffer::readData(ptrdiff_t offsetElementCount,
                               ptrdiff_t rangeElementCount,
                               bufferPtr result) const {

    if (rangeElementCount > 0) {
        ptrdiff_t rangeInBytes = rangeElementCount * _elementSize;
        ptrdiff_t offsetInBytes = offsetElementCount * _elementSize;

        assert(offsetInBytes + rangeInBytes <= (ptrdiff_t)_allignedBufferSize &&
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

void glUniformBuffer::writeData(ptrdiff_t offsetElementCount,
                                ptrdiff_t rangeElementCount,
                                const bufferPtr data) {

    writeBytes(offsetElementCount * _elementSize,
               rangeElementCount * _elementSize,
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

    size_t req = alignmentRequirement(_usage);
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

    dataOut._bufferGUID = bufferImpl()->getGUID();
    dataOut._range = static_cast<size_t>(rangeElementCount * _elementSize);
    dataOut._offset = static_cast<size_t>(offsetElementCount * _elementSize);
    if (queueLength() > 1) {
        dataOut._offset += static_cast<size_t>(queueReadIndex() * _allignedBufferSize);
    }
    size_t req = alignmentRequirement(_usage);
    if (dataOut._offset % req != 0) {
        dataOut._offset = (dataOut._offset + req - 1) / req * req;
    }
    dataOut._flush = BitCompare(_flags, ShaderBuffer::Flags::ALLOW_THREADED_WRITES);

    assert(dataOut._range <= _maxSize &&
           "glUniformBuffer::bindRange: attempted to bind a larger shader block than is allowed on the current platform");

    assert(dataOut._offset % req == 0);
    return _buffer->bindRange(bindIndex, dataOut._offset, dataOut._range);
}

bool glUniformBuffer::bind(U8 bindIndex) {
    return bindRange(bindIndex, 0, _elementCount);
}

void glUniformBuffer::printInfo(const ShaderProgram* shaderProgram, U8 bindIndex) {
    GLuint prog = shaderProgram->getID();
    GLuint block_index = bindIndex;

    if (prog <= 0 || _usage != ShaderBuffer::Usage::CONSTANT_BUFFER) {
        return;
    }

    // Fetch uniform block name:
    GLint name_length;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_NAME_LENGTH,
                              &name_length);
    if (name_length <= 0) {
        return;
    }

    stringImpl block_name(name_length, 0);
    glGetActiveUniformBlockName(prog, block_index, name_length, NULL,
                                &block_name[0]);

    GLint block_size;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_DATA_SIZE,
                              &block_size);
    // Fetch info on each active uniform:
    GLint active_uniforms = 0;
    glGetActiveUniformBlockiv(
        prog, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &active_uniforms);

    vector<GLuint> uniform_indices(active_uniforms, 0);
    glGetActiveUniformBlockiv(prog, block_index,
                              GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                              reinterpret_cast<GLint*>(uniform_indices.data()));

    vector<GLint> name_lengths(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_NAME_LENGTH,
                          &name_lengths[0]);

    vector<GLint> offsets(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_OFFSET, &offsets[0]);

    vector<GLint> types(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_TYPE, &types[0]);

    vector<GLint> sizes(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_SIZE, &sizes[0]);

    vector<GLint> strides(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_ARRAY_STRIDE,
                          &strides[0]);

    // Build a string detailing each uniform in the block:
    vector<stringImpl> uniform_details;
    uniform_details.reserve(uniform_indices.size());
    for (vec_size i = 0; i < uniform_indices.size(); ++i) {
        GLuint const uniform_index = uniform_indices[i];

        stringImpl name(name_lengths[i], 0);
        glGetActiveUniformName(prog, uniform_index, name_lengths[i], NULL,
                               &name[0]);

        ostringstreamImpl details;
        details << std::setfill('0') << std::setw(4) << offsets[i] << ": "
                << std::setfill(' ') << std::setw(5) << types[i] << " "
                << name;

        if (sizes[i] > 1) {
            details << "[" << sizes[i] << "]";
        }

        details << "\n";

        uniform_details.push_back(details.str());
    }

    // Sort uniform detail string alphabetically. (Since the detail strings
    // start with the uniform's byte offset, this will order the uniforms in
    // the order they are laid out in memory:
    std::sort(std::begin(uniform_details), std::end(uniform_details));

    // Output details:
    Console::printfn("%s ( %d )", block_name.c_str(), block_size);
    for (vector<stringImpl>::iterator detail = std::begin(uniform_details);
         detail != std::end(uniform_details); ++detail) {
        Console::printfn("%s", (*detail).c_str());
    }
}

void glUniformBuffer::onGLInit() {
    ShaderBuffer::s_boundAlignmentRequirement = GL_API::s_UBOffsetAlignment;
    ShaderBuffer::s_unboundAlignmentRequirement = GL_API::s_SSBOffsetAlignment;
}

glBufferImpl* glUniformBuffer::bufferImpl() const {
    return _buffer;
}

};  // namespace Divide
