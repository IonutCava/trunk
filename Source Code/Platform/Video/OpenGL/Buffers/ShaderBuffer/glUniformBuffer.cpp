#include "Headers/glUniformBuffer.h"

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

vec4<U32> glUniformBuffer::_currentBindConfig(Config::PRIMITIVE_RESTART_INDEX_L);

glUniformBuffer::glUniformBuffer(const stringImpl& bufferName, bool unbound,
                                 bool persistentMapped)
    : ShaderBuffer(bufferName, unbound, persistentMapped),
      _mappedBuffer(nullptr),
      _UBOid(0),
      _lockManager(_persistentMapped
                       ? MemoryManager_NEW glBufferLockManager(true)
                       : nullptr),

      _target(_unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER)
{
   if (ShaderBuffer::_targetDataAlignment[0] == -1) {
        ShaderBuffer::_targetDataAlignment[0] = ParamHandler::getInstance().getParam<I32>("rendering.SSBOAligment", 256);
    }
    if (ShaderBuffer::_targetDataAlignment[1] == -1) {
        ShaderBuffer::_targetDataAlignment[1] = ParamHandler::getInstance().getParam<I32>("rendering.UBOAligment", 256);
    }
}

glUniformBuffer::~glUniformBuffer() 
{
    Destroy();
}

void glUniformBuffer::Destroy() {
    if (_persistentMapped) {
        _lockManager->WaitForLockedRange(0, _bufferSize);
        DiscardAllData();
    }
    GLUtil::freeBuffer(_UBOid, _mappedBuffer);
}

void glUniformBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    DIVIDE_ASSERT(
        _UBOid == 0,
        "glUniformBuffer::Create error: Tried to double create current UBO");

    ShaderBuffer::Create(primitiveCount, primitiveSize);

    if (_persistentMapped) {
        BufferStorageMask storage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
                                   GL_MAP_COHERENT_BIT;
        BufferAccessMask access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
                                  GL_MAP_COHERENT_BIT;

        I32 remainder = _bufferSize % ShaderBuffer::getTargetDataAlignment(_unbound);
        if (remainder > 0) {
            _primitiveCount += static_cast<U32>(
                (ShaderBuffer::getTargetDataAlignment(_unbound) - remainder) /
                _primitiveSize);
            _bufferSize = _primitiveCount * _primitiveSize;
        }

        _mappedBuffer = GLUtil::createAndAllocPersistentBuffer(
            _bufferSize, storage, access, _UBOid);

        DIVIDE_ASSERT(_mappedBuffer != nullptr,
                        "glUniformBuffer::Create error: "
                        "Can't mapped persistent buffer!");
        
    } else {
        GLUtil::createAndAllocBuffer(_bufferSize, GL_DYNAMIC_DRAW, _UBOid);
    }
}

void glUniformBuffer::DiscardAllData() const {
    glInvalidateBufferData(_UBOid);
}

void glUniformBuffer::DiscardSubData(ptrdiff_t offsetElementCount,
                                     ptrdiff_t rangeElementCount) const {
    DIVIDE_ASSERT((offsetElementCount + rangeElementCount) * _primitiveSize <=
                      _bufferSize,
                  "glUniformBuffer error: DiscardSubData was called with an "
                  "invalid range (buffer overflow)!");
    glInvalidateBufferSubData(_UBOid, offsetElementCount * _primitiveSize,
                              rangeElementCount * _primitiveSize);
}

void glUniformBuffer::UpdateData(GLintptr offsetElementCount, GLsizeiptr rangeElementCount,
                                 const bufferPtr data) const {

    if (offsetElementCount == rangeElementCount) {
        return;
    }

    GLintptr offset = offsetElementCount * _primitiveSize;
    GLintptr range = rangeElementCount * _primitiveSize;

    DIVIDE_ASSERT(offset + range <= (GLsizeiptr)_bufferSize,
                  "glUniformBuffer::UpdateData error: was called with an "
                  "invalid range (buffer overflow)!");
    DIVIDE_ASSERT(_persistentMapped == (_mappedBuffer != nullptr),
                  "glUniformBuffer::UpdateData error: was called for an "
                  "unmapped buffer!");

    //DiscardSubData(offsetElementCount, rangeElementCount);

    if (_persistentMapped) {
        _lockManager->WaitForLockedRange(offset, range);
        memcpy(reinterpret_cast<U8*>(_mappedBuffer) + offset, data, range);
        _lockManager->LockRange(offset, range);
    } else {
        GLUtil::updateBuffer(_UBOid, offset, range, data);
    }
}

bool glUniformBuffer::BindRange(U32 bindIndex, U32 offsetElementCount,
                                U32 rangeElementCount) {
    if (CheckBindRange(bindIndex, offsetElementCount, rangeElementCount)) {
        DIVIDE_ASSERT(
            _UBOid != 0,
            "glUniformBuffer error: Tried to bind an uninitialized UBO");

        _currentBindConfig.set(bindIndex, _UBOid, offsetElementCount,
                               rangeElementCount);

        if (rangeElementCount != 0) {
            size_t range = _primitiveSize * rangeElementCount;
            I32 remainder = range % ShaderBuffer::getTargetDataAlignment(_unbound);

            glBindBufferRange(
                _target, bindIndex, _UBOid, _primitiveSize * offsetElementCount,
                remainder ? range + (ShaderBuffer::getTargetDataAlignment(_unbound) - remainder)
                          : range);
        }

        return true;
    }

    return false;
}

bool glUniformBuffer::CheckBindRange(U32 bindIndex, U32 offsetElementCount,
                                     U32 rangeElementCount) {
    return _currentBindConfig !=
           vec4<U32>(bindIndex, _UBOid, offsetElementCount, rangeElementCount);
}

bool glUniformBuffer::CheckBind(U32 bindIndex) {
    return CheckBindRange(bindIndex, 0, _primitiveCount);
}

bool glUniformBuffer::Bind(U32 bindIndex) {
    return BindRange(bindIndex, 0, _primitiveCount);
}

void glUniformBuffer::PrintInfo(const ShaderProgram* shaderProgram,
                                U32 bindIndex) {
    GLuint prog = shaderProgram->getID();
    GLuint block_index = bindIndex;

    if (prog <= 0 || block_index < 0 || _unbound) {
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
                << name.c_str();

        if (sizes[i] > 1) {
            details << "[" << sizes[i] << "]";
        }

        details << "\n";
        uniform_details.push_back(stringAlg::toBase(details.str()));
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