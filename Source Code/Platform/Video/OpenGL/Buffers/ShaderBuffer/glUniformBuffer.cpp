#include "Headers/glUniformBuffer.h"

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include <iomanip> 

namespace Divide {

glUniformBuffer::glUniformBuffer(bool unbound, bool persistentMapped) : ShaderBuffer(unbound, persistentMapped), 
                                                                        _mappedBuffer(nullptr),
                                                                        _lockManager(nullptr),
                                                                        _UBOid(0)
{
    if (Config::Profile::DISABLE_PERSISTENT_BUFFER) {
        persistentMapped = _persistentMapped = false;
    }

    if (persistentMapped) {
        _lockManager = MemoryManager_NEW glBufferLockManager(true);
    }
}

glUniformBuffer::~glUniformBuffer()
{
    if (_UBOid > 0) {
        if (_persistentMapped) {
            GL_API::setActiveBuffer(_target, _UBOid);
            glUnmapBuffer(_target);
        }
        glDeleteBuffers(1, &_UBOid);
        _UBOid = 0;
    }

    MemoryManager::DELETE( _lockManager );
}

void glUniformBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    DIVIDE_ASSERT(_UBOid == 0, "glUniformBuffer error: Tried to double create current UBO");
    ShaderBuffer::Create(primitiveCount, primitiveSize);

    glGenBuffers(1, &_UBOid); // Generate the buffer
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: UBO creation failed");
    
    _target = _unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;

    if (_persistentMapped) {
        GLenum usageFlag = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glNamedBufferStorageEXT(_UBOid, _bufferSize, NULL, usageFlag | GL_DYNAMIC_STORAGE_BIT);
        _mappedBuffer = glMapNamedBufferRangeEXT(_UBOid, 0, _bufferSize, usageFlag);
    }else{
        glNamedBufferDataEXT(_UBOid, _bufferSize, NULL, GL_DYNAMIC_DRAW);
    }
}

void glUniformBuffer::DiscardAllData() {
    glInvalidateBufferData(_UBOid);
}

void glUniformBuffer::DiscardSubData(ptrdiff_t offset, ptrdiff_t size) {
    DIVIDE_ASSERT(offset + size <= (GLsizeiptr)_bufferSize, 
                  "glUniformBuffer error: DiscardSubData was called with an invalid range (buffer overflow)!");
    glInvalidateBufferSubData(_UBOid, offset, size);
}

void glUniformBuffer::UpdateData(GLintptr offset, GLsizeiptr size, const void *data, const bool invalidateBuffer) const {
    DIVIDE_ASSERT(offset + size <= (GLsizeiptr)_bufferSize, 
                  "glUniformBuffer error: ChangeSubData was called with an invalid range (buffer overflow)!");

    if (invalidateBuffer) {
        if (_persistentMapped) {
            //glNamedBufferSubDataEXT(_UBOid, 0, _bufferSize, NULL);
        } else {
            glNamedBufferDataEXT(_UBOid, _bufferSize, NULL, GL_DYNAMIC_DRAW);
        }
    }

    if (size == 0 || !data) {
        return;
    }

    if (_persistentMapped) {
        GL_API::setActiveBuffer(_target, _UBOid);
        _lockManager->WaitForLockedRange(offset, size);
            void* dst = (U8*) _mappedBuffer + offset;
            memcpy(dst, data, size);
        _lockManager->LockRange(offset, size);
    } else {
        glNamedBufferSubDataEXT(_UBOid, offset, size, data);
    }
}

bool glUniformBuffer::BindRange(ShaderBufferLocation bindIndex, U32 offsetElementCount, U32 rangeElementCount) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");

    glBindBufferRange(_target, bindIndex, _UBOid, _primitiveSize * offsetElementCount, _primitiveSize * rangeElementCount);

    return true;
}

bool glUniformBuffer::Bind(ShaderBufferLocation bindIndex) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");

    glBindBufferBase(_target, bindIndex, _UBOid);

    return true;
}

void glUniformBuffer::PrintInfo(const ShaderProgram* shaderProgram, ShaderBufferLocation bindIndex) {
    GLuint prog = shaderProgram->getId();
    GLuint block_index = bindIndex;

    if (prog <= 0 || block_index < 0 || _unbound) {
        return;
    }

    // Fetch uniform block name:
    GLint name_length;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_length);
    if (name_length <= 0) {
        return;
    }

    stringImpl block_name(name_length, 0);
    glGetActiveUniformBlockName(prog, block_index, name_length, NULL, &block_name[0]);

    GLint block_size;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
    // Fetch info on each active uniform:
    GLint active_uniforms = 0;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &active_uniforms);
    
    vectorImpl<GLuint> uniform_indices(active_uniforms, 0);
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, 
                              reinterpret_cast<GLint*>(uniform_indices.data()));

    vectorImpl<GLint> name_lengths(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_NAME_LENGTH, &name_lengths[0]);

    vectorImpl<GLint> offsets(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_OFFSET, &offsets[0]);

    vectorImpl<GLint> types(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_TYPE, &types[0]);

    vectorImpl<GLint> sizes(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_SIZE, &sizes[0]);

    vectorImpl<GLint> strides(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_ARRAY_STRIDE, &strides[0]);

    // Build a string detailing each uniform in the block:
    vectorImpl<stringImpl> uniform_details;
    uniform_details.reserve(uniform_indices.size());
    for (vectorAlg::vecSize i = 0; i < uniform_indices.size(); ++i) {
        GLuint const uniform_index = uniform_indices[i];

        stringImpl name(name_lengths[i], 0);
        glGetActiveUniformName(prog, uniform_index, name_lengths[i], NULL, &name[0]);

        std::ostringstream details;
        details << 
        std::setfill('0') << 
        std::setw(4) << 
        offsets[i] << 
        ": " << 
        std::setfill(' ') << 
        std::setw(5) << 
        types[i] << 
        " " << 
        name.c_str();

        if (sizes[i] > 1) {
            details << "[" << sizes[i] << "]";
        }

        details << "\n";
        uniform_details.push_back(stringAlg::toBase(details.str()));
    }

    // Sort uniform detail string alphabetically. (Since the detail strings 
    // start with the uniform's byte offset, this will order the uniforms in 
    // the order they are laid out in memory:
    std::sort(uniform_details.begin(), uniform_details.end());

    // Output details:
    PRINT_FN("%s ( %d )", block_name.c_str(), block_size);
    for (auto detail = uniform_details.begin(); detail != uniform_details.end(); ++detail) {
        PRINT_FN("%s", (*detail).c_str());
    }
}

}; //namespace Divide