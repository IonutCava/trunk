#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Headers/glUniformBuffer.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "core.h"

glUniformBuffer::glUniformBuffer(const bool unbound) : ShaderBuffer(unbound), _UBOid(0), _bufferSize(0)
{
}

glUniformBuffer::~glUniformBuffer()
{
    if(_UBOid > 0) {
        glDeleteBuffers(1, &_UBOid);
        _UBOid = 0;
    }
}

void glUniformBuffer::printInfo(const ShaderProgram* shaderProgram, U32 bindIndex) {
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

void glUniformBuffer::Create(bool dynamic, bool stream){
    DIVIDE_ASSERT(_UBOid == 0, "glUniformBuffer error: Tried to double create current UBO");

    _usage = (dynamic ? (stream ? GL_STREAM_DRAW : GL_DYNAMIC_DRAW) : GL_STATIC_DRAW);
    glGenBuffers(1, &_UBOid); // Generate the buffer
    _target = _unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;

    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: UBO creation failed");
}

void glUniformBuffer::setActive() const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to modify an uninitialized UBO");
    _unbound ? GL_API::setActiveBuffer(GL_SHADER_STORAGE_BUFFER, _UBOid) : GL_API::setActiveBuffer(GL_UNIFORM_BUFFER, _UBOid);
}

void glUniformBuffer::ReserveBuffer(GLuint primitiveCount, GLsizeiptr primitiveSize) {
    setActive();
    glBufferData(_target, primitiveSize * primitiveCount, nullptr, _usage);
    _bufferSize = primitiveSize * primitiveCount;
}

void glUniformBuffer::ChangeSubData(GLintptr offset, GLsizeiptr size, const GLvoid *data, const bool invalidateBuffer) const {
    setActive();
    if(invalidateBuffer){
        DIVIDE_ASSERT(size == _bufferSize, "glUniformBuffer error: Buffer orphaning on ChangeSubData is available only if the entire buffer data is replaced!");
        glBufferData(_target, _bufferSize, NULL, _usage);
    }
    DIVIDE_ASSERT(offset + size <= _bufferSize, "glUniformBuffer error: ChangeSubData was called with an invalid range (buffer overflow)!");
    glBufferSubData(_target, offset, size, data);
}

bool glUniformBuffer::bindRange(GLuint bindIndex, GLintptr offset, GLsizeiptr size) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");
    glBindBufferRange(_target, bindIndex, _UBOid, offset, size);
    return true;
}

bool glUniformBuffer::bind(GLuint bindIndex) const {
    DIVIDE_ASSERT(_UBOid != 0, "glUniformBuffer error: Tried to bind an uninitialized UBO");
    _unbound ? glBindBufferRange(_target, bindIndex, _UBOid, 0, _bufferSize) : glBindBufferBase(_target, bindIndex, _UBOid);
    return true;
}