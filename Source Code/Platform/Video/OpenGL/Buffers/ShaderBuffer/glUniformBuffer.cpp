#include "Headers/glUniformBuffer.h"

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

namespace {

    typedef std::array<vec3<U32>, to_const_uint(ShaderBufferLocation::COUNT)> BindConfig;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
                                 U32 bindIndex,
                                 U32 offsetElementCount,
                                 U32 rangeElementCount) {

        vec3<U32>& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig.x != UBOid ||
            crtConfig.y != offsetElementCount ||
            crtConfig.z != rangeElementCount) {
            crtConfig.set(UBOid, offsetElementCount, rangeElementCount);
            return true;
        }

        return false;
    }
};

glUniformBuffer::glUniformBuffer(const stringImpl& bufferName,
                                 const U32 sizeFactor,
                                 bool unbound,
                                 bool persistentMapped,
                                 BufferUpdateFrequency frequency)
    : ShaderBuffer(bufferName, sizeFactor, unbound, persistentMapped, frequency),
      _UBOid(0),
      _mappedBuffer(nullptr),
      _alignment(0),
      _target(_unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER),
      _lockManager(_persistentMapped
                       ? std::make_unique<glBufferLockManager>(true)
                       : nullptr)

{
    _updated = false;
    _alignment = _unbound ? ParamHandler::getInstance().getParam<I32>("rendering.SSBOAligment", 32)
                          : ParamHandler::getInstance().getParam<I32>("rendering.UBOAligment", 32);
}

glUniformBuffer::~glUniformBuffer() 
{
    destroy();
}

void glUniformBuffer::destroy() {
    if (_persistentMapped) {
        _lockManager->WaitForLockedRange(0, _allignedBufferSize);
    } else {
        glInvalidateBufferData(_UBOid);
    }
    GLUtil::freeBuffer(_UBOid, _mappedBuffer);
}

void glUniformBuffer::create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    DIVIDE_ASSERT(
        _UBOid == 0,
        "glUniformBuffer::Create error: Tried to double create current UBO");

    ShaderBuffer::create(primitiveCount, primitiveSize);

    _allignedBufferSize = realign_offset(_bufferSize, _alignment);

    if (_persistentMapped) {
        _mappedBuffer = 
            GLUtil::createAndAllocPersistentBuffer(_allignedBufferSize * queueLength(),
                                                   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT,
                                                   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT,
                                                   _UBOid);

        DIVIDE_ASSERT(_mappedBuffer != nullptr,
                        "glUniformBuffer::Create error: "
                        "Can't mapped persistent buffer!");
        
    } else {
        GLenum _usage = _frequency == BufferUpdateFrequency::ONCE 
                                    ? GL_STATIC_DRAW
                                    : _frequency == BufferUpdateFrequency::OCASSIONAL
                                                  ? GL_DYNAMIC_DRAW
                                                  : GL_STREAM_DRAW;
        GLUtil::createAndAllocBuffer(_allignedBufferSize * queueLength(),
                                      _usage,
                                     _UBOid);
    }
}

void glUniformBuffer::updateData(GLintptr offsetElementCount, GLsizeiptr rangeElementCount, const bufferPtr data) {

    if (rangeElementCount == 0) {
        return;
    }

    DIVIDE_ASSERT(_persistentMapped == (_mappedBuffer != nullptr),
                  "glUniformBuffer::UpdateData error: was called for an "
                  "unmapped buffer!");

    GLintptr range = rangeElementCount * _primitiveSize;
    GLintptr offset = offsetElementCount * _primitiveSize;

    DIVIDE_ASSERT(offset + range <= (GLsizeiptr)_bufferSize,
        "glUniformBuffer::UpdateData error: was called with an "
        "invalid range (buffer overflow)!");

    offset += _queueWriteIndex * _allignedBufferSize;

    if (_persistentMapped) {
        _lockManager->WaitForLockedRange(offset, range);
        memcpy((U8*)(_mappedBuffer) + offset, data, range);
    } else {
        //glInvalidateBufferSubData(_UBOid, offset, range);
        glNamedBufferSubData(_UBOid, offset, range, data);
    }
}

bool glUniformBuffer::bindRange(U32 bindIndex, U32 offsetElementCount, U32 rangeElementCount) {
    DIVIDE_ASSERT(_UBOid != 0,
                  "glUniformBuffer error: Tried to bind an uninitialized UBO");

    if (rangeElementCount == 0)  {
        return false;
    }

    size_t range = rangeElementCount * _primitiveSize;
    size_t offset = offsetElementCount * _primitiveSize;
    offset += _queueReadIndex * _allignedBufferSize;

    bool success = false;
    if (setIfDifferentBindRange(_UBOid, bindIndex, offsetElementCount, rangeElementCount)) {
        glBindBufferRange(_target, bindIndex, _UBOid, offset, range);
        success = true;
    }

    if (_persistentMapped) {
        _lockManager->LockRange(offset, range);
    }

    return success;
}

bool glUniformBuffer::bind(U32 bindIndex) {
    return bindRange(bindIndex, 0, _primitiveCount);
}

void glUniformBuffer::addAtomicCounter(U32 sizeFactor) {
    vectorImpl<GLuint> data(sizeFactor, 0);

    GLuint atomicsBuffer;
    glCreateBuffers(1, &atomicsBuffer);
    glNamedBufferData(atomicsBuffer, sizeof(GLuint) * sizeFactor, data.data(), GL_DYNAMIC_COPY);

    _atomicCounters.push_back({atomicsBuffer, sizeFactor, 0, 1 % sizeFactor});
}

U32 glUniformBuffer::getAtomicCounter(U32 counterIndex) {
    if (counterIndex > to_uint(_atomicCounters.size())) {
        return 0;
    }

    GLuint result = 0;
    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    glGetNamedBufferSubData(counter._handle, counter._readHead * sizeof(GLuint), sizeof(GLuint), &result);
    return result;
}

void glUniformBuffer::bindAtomicCounter(U32 counterIndex, U32 bindIndex) {
    if (counterIndex > to_uint(_atomicCounters.size())) {
        return;
    }
    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, bindIndex, counter._handle);
}

void glUniformBuffer::resetAtomicCounter(U32 counterIndex) {
    if (counterIndex > to_uint(_atomicCounters.size())) {
        return;
    }
    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    vectorImpl<GLuint> data(counter._sizeFactor, 0);
    glNamedBufferSubData(counter._handle, 0, sizeof(GLuint) * counter._sizeFactor, data.data());
}

void glUniformBuffer::printInfo(const ShaderProgram* shaderProgram,
                                U32 bindIndex) {
    GLuint prog = shaderProgram->getID();
    GLuint block_index = bindIndex;

    if (prog <= 0 || block_index < 0 || _unbound) {
        return;
    }

    if (_persistentMapped) {
        _lockManager->WaitForLockedRange(0, _bufferSize);
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

    vectorImpl<GLuint> uniform_indices(active_uniforms, 0);
    glGetActiveUniformBlockiv(prog, block_index,
                              GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                              reinterpret_cast<GLint*>(uniform_indices.data()));

    vectorImpl<GLint> name_lengths(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_NAME_LENGTH,
                          &name_lengths[0]);

    vectorImpl<GLint> offsets(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_OFFSET, &offsets[0]);

    vectorImpl<GLint> types(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_TYPE, &types[0]);

    vectorImpl<GLint> sizes(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_SIZE, &sizes[0]);

    vectorImpl<GLint> strides(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(),
                          &uniform_indices[0], GL_UNIFORM_ARRAY_STRIDE,
                          &strides[0]);

    // Build a string detailing each uniform in the block:
    vectorImpl<stringImpl> uniform_details;
    uniform_details.reserve(uniform_indices.size());
    for (vectorAlg::vecSize i = 0; i < uniform_indices.size(); ++i) {
        GLuint const uniform_index = uniform_indices[i];

        stringImpl name(name_lengths[i], 0);
        glGetActiveUniformName(prog, uniform_index, name_lengths[i], NULL,
                               &name[0]);

        std::ostringstream details;
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
    for (vectorImpl<stringImpl>::iterator detail = std::begin(uniform_details);
         detail != std::end(uniform_details); ++detail) {
        Console::printfn("%s", (*detail).c_str());
    }
}

};  // namespace Divide
