#include "Headers/glUniformBuffer.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "core.h"

glUniformBuffer::glUniformBuffer(bool unbound, bool persistentMapped) : ShaderBuffer(unbound, persistentMapped), _UBOid(0), _mappedBuffer(nullptr)
{
    _lockManager = persistentMapped ? New glBufferLockManager(true) : nullptr;
}

glUniformBuffer::~glUniformBuffer()
{
    if(_UBOid > 0) {
        if(_persistentMapped) {
            GL_API::setActiveBuffer(_target, _UBOid);
            glUnmapBuffer(_target);
        }
        glDeleteBuffers(1, &_UBOid);
        _UBOid = 0;
    }
    SAFE_DELETE(_lockManager);
}

void glUniformBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
    DIVIDE_ASSERT(_UBOid == 0, "glUniformBuffer error: Tried to double create current UBO");

    glGenBuffers(1, &_UBOid); // Generate the buffer
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: UBO creation failed");

    _target = _unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;

    GL_API::setActiveBuffer(_target, _UBOid);
    if(_persistentMapped) {
        GLenum usageFlag = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(_target, primitiveSize * primitiveCount, NULL, usageFlag | GL_DYNAMIC_STORAGE_BIT);
        _mappedBuffer = glMapBufferRange(_target, 0, primitiveSize * primitiveCount, usageFlag);
    }else{
        glBufferData(_target, primitiveSize * primitiveCount, NULL, GL_DYNAMIC_DRAW);
    }
    ShaderBuffer::Create(primitiveCount, primitiveSize);
}

void glUniformBuffer::UpdateData(GLintptr offset, GLsizeiptr size, const GLvoid *data, const bool invalidateBuffer) const {
    DIVIDE_ASSERT(offset + size <= (GLsizeiptr)_bufferSize, "glUniformBuffer error: ChangeSubData was called with an invalid range (buffer overflow)!");

    if(invalidateBuffer)
        glInvalidateBufferData(_UBOid);
    
    if(size == 0 || !data)
        return;

    if(_persistentMapped) {
        GL_API::setActiveBuffer(_target, _UBOid);
        _lockManager->WaitForLockedRange(offset, size);
            void* dst = (U8*) _mappedBuffer + offset;
            memcpy(dst, data, size);
        _lockManager->LockRange(offset, size);
    }else{
        glNamedBufferSubDataEXT(_UBOid, offset, size, data);
    }
}

bool glUniformBuffer::BindRange(Divide::ShaderBufferLocation bindIndex, U32 offsetElementCount, U32 rangeElementCount) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");
    if(_persistentMapped)
        _lockManager->WaitForLockedRange();
    glBindBufferRange(_target, bindIndex, _UBOid, _primitiveSize * offsetElementCount, _primitiveSize * rangeElementCount);
    return true;
}

bool glUniformBuffer::Bind(Divide::ShaderBufferLocation bindIndex) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");
    if(_persistentMapped)
        _lockManager->WaitForLockedRange();
    glBindBufferBase(_target, bindIndex, _UBOid);
    return true;
}

void glUniformBuffer::PrintInfo(const ShaderProgram* shaderProgram, Divide::ShaderBufferLocation bindIndex) {
    GLuint prog = shaderProgram->getId();
    GLuint block_index = bindIndex;

    if (prog <= 0 || block_index < 0 || _unbound)
        return;

    // Fetch uniform block name:
    GLint name_length;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_length);
    if (name_length <= 0)
        return;

    std::string block_name(name_length, 0);
    glGetActiveUniformBlockName(prog, block_index, name_length, NULL, &block_name[0]);

    GLint block_size;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
    // Fetch info on each active uniform:
    GLint active_uniforms = 0;
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &active_uniforms);
    
    std::vector<GLuint> uniform_indices(active_uniforms, 0);
    glGetActiveUniformBlockiv(prog, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, reinterpret_cast<GLint*>(&uniform_indices[0]));

    std::vector<GLint> name_lengths(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_NAME_LENGTH, &name_lengths[0]);

    std::vector<GLint> offsets(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_OFFSET, &offsets[0]);

    std::vector<GLint> types(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_TYPE, &types[0]);

    std::vector<GLint> sizes(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_SIZE, &sizes[0]);

    std::vector<GLint> strides(uniform_indices.size(), 0);
    glGetActiveUniformsiv(prog, (GLsizei)uniform_indices.size(), &uniform_indices[0], GL_UNIFORM_ARRAY_STRIDE, &strides[0]);

    // Build a string detailing each uniform in the block:
    std::vector<std::string> uniform_details;
    uniform_details.reserve(uniform_indices.size());
    for (std::size_t i = 0; i < uniform_indices.size(); ++i)
    {
        GLuint const uniform_index = uniform_indices[i];

        std::string name(name_lengths[i], 0);
        glGetActiveUniformName(prog, uniform_index, name_lengths[i], NULL, &name[0]);

        std::ostringstream details;
        details << std::setfill('0') << std::setw(4) << offsets[i] << ": " << std::setfill(' ') << std::setw(5) << types[i] << " " << name;

        if (sizes[i] > 1)
            details << "[" << sizes[i] << "]";

        details << "\n";
        uniform_details.push_back(details.str());
    }

    // Sort uniform detail string alphabetically. (Since the detail strings 
    // start with the uniform's byte offset, this will order the uniforms in 
    // the order they are laid out in memory:
    std::sort(uniform_details.begin(), uniform_details.end());

    // Output details:
    PRINT_FN("%s ( %d )", block_name.c_str(), block_size);
    for (auto detail = uniform_details.begin(); detail != uniform_details.end(); ++detail)
        PRINT_FN("%s", (*detail).c_str());
}
