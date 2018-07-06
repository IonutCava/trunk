#include "Headers/glUniformBuffer.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferLockManager.h"
#include "Core/Headers/ParamHandler.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    typedef std::pair<GLuint, GLuint> AtomicBufferBindConfig;

    hashMapImpl<GLuint, AtomicBufferBindConfig> g_currentBindConfig;

    bool SetIfDifferentBindRange(GLuint bindIndex, GLuint buffer, GLuint bindOffset) {
        AtomicBufferBindConfig targetCfg = std::make_pair(buffer, bindOffset);
        // If this is a new index, this will just create a default config
        AtomicBufferBindConfig& crtConfig = g_currentBindConfig[bindIndex];
        if (targetCfg == crtConfig) {
            return false;
        }

        crtConfig = targetCfg;
        glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, bindIndex, buffer, bindOffset, sizeof(GLuint));
        return true;
    }
};

IMPLEMENT_CUSTOM_ALLOCATOR(glUniformBuffer, 0, 0)
glUniformBuffer::glUniformBuffer(GFXDevice& context,
                                 const U32 ringBufferLength,
                                 bool unbound,
                                 bool persistentMapped,
                                 BufferUpdateFrequency frequency)
    : ShaderBuffer(context, ringBufferLength, unbound, persistentMapped, frequency),
      _mappedBuffer(nullptr),
      _alignment(0),
      _allignedBufferSize(0),
      _target(_unbound ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER)
{
    _updated = false;
    _alignmentRequirement = _unbound ? ParamHandler::instance().getParam<I32>(_ID("rendering.SSBOAligment"), 32)
                                     : ParamHandler::instance().getParam<I32>(_ID("rendering.UBOAligment"), 32);
    if (persistentMapped) {
        _buffer = MemoryManager_NEW glPersistentBuffer(_target);
    } else {
        _buffer = MemoryManager_NEW glRegularBuffer(_target);
    }
}

glUniformBuffer::~glUniformBuffer() 
{
    destroy();
    MemoryManager::DELETE(_buffer);
}

GLuint glUniformBuffer::bufferID() const {
    return _buffer->bufferID();
}

void glUniformBuffer::destroy() {
    _buffer->destroy();
    for (AtomicCounter& counter : _atomicCounters) {
        glDeleteBuffers(1, &counter._handle);
    }
    _atomicCounters.clear();
}

void glUniformBuffer::create(U32 primitiveCount, ptrdiff_t primitiveSize, U32 sizeFactor) {
    ShaderBuffer::create(primitiveCount, primitiveSize, sizeFactor);
    _allignedBufferSize = realign_offset(_bufferSize, _alignmentRequirement);
    _buffer->create(_frequency, _allignedBufferSize * sizeFactor * queueLength());
}

void glUniformBuffer::getData(ptrdiff_t offsetElementCount,
                              ptrdiff_t rangeElementCount,
                              bufferPtr result) const {
    if (rangeElementCount > 0) {
        ptrdiff_t range = rangeElementCount * _primitiveSize;
        ptrdiff_t offset = offsetElementCount * _primitiveSize;

        assert(offset + range <= (ptrdiff_t)_allignedBufferSize &&
            "glUniformBuffer::UpdateData error: was called with an "
            "invalid range (buffer overflow)!");

        offset += queueWriteIndex() * _allignedBufferSize;

        glGetNamedBufferSubData(bufferID(), offset, range, result);
    }
}

void glUniformBuffer::updateData(ptrdiff_t offsetElementCount,
                                 ptrdiff_t rangeElementCount,
                                 const bufferPtr data) {

    if (rangeElementCount == 0) {
        return;
    }

    ptrdiff_t range = rangeElementCount * _primitiveSize;
    ptrdiff_t offset = offsetElementCount * _primitiveSize;

    DIVIDE_ASSERT(offset + range <= (ptrdiff_t)_allignedBufferSize,
        "glUniformBuffer::UpdateData error: was called with an "
        "invalid range (buffer overflow)!");

    offset += queueWriteIndex() * _allignedBufferSize;

    _buffer->updateData(offset, range, data);
}

bool glUniformBuffer::bindRange(U32 bindIndex, U32 offsetElementCount, U32 rangeElementCount) {
    if (rangeElementCount == 0)  {
        return false;
    }

    GLuint range = static_cast<GLuint>(rangeElementCount * _primitiveSize);
    GLuint offset = static_cast<GLuint>(offsetElementCount * _primitiveSize);
    offset += static_cast<GLuint>(queueReadIndex() * _allignedBufferSize);

    return _buffer->bindRange(bindIndex, offset, range);
}

bool glUniformBuffer::bind(U32 bindIndex) {
    return bindRange(bindIndex, 0, _primitiveCount);
}

void glUniformBuffer::addAtomicCounter(U32 sizeFactor) {
    vectorImpl<GLuint> data(sizeFactor, 0);

    GLuint atomicsBuffer;
    glCreateBuffers(1, &atomicsBuffer);

    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_BUFFER,
                      atomicsBuffer,
                      -1,
                      Util::StringFormat("DVD_ATOMIC_BUFFER_%d", atomicsBuffer).c_str());
    }

    glNamedBufferData(atomicsBuffer, sizeof(GLuint) * sizeFactor, data.data(), GL_DYNAMIC_READ);
    _atomicCounters.push_back({atomicsBuffer, sizeFactor, 0});
}

U32 glUniformBuffer::getAtomicCounter(U32 counterIndex) {
    if (counterIndex >= to_uint(_atomicCounters.size())) {
        return 0;
    }

    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    GLuint readHead = counter._sizeFactor - counter._writeHead - 1;
    GLuint result = 0;
    glGetNamedBufferSubData(counter._handle, readHead * sizeof(GLuint), sizeof(GLuint), &result);

    return result;
}

void glUniformBuffer::bindAtomicCounter(U32 counterIndex, U32 bindIndex) {
    if (counterIndex >= to_uint(_atomicCounters.size())) {
        return;
    }

    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    SetIfDifferentBindRange(bindIndex, counter._handle, counter._writeHead * sizeof(GLuint));
    counter._writeHead = (counter._writeHead + 1) % counter._sizeFactor;
}

void glUniformBuffer::resetAtomicCounter(U32 counterIndex) {
    if (counterIndex > to_uint(_atomicCounters.size())) {
        return;
    }
    AtomicCounter& counter = _atomicCounters.at(counterIndex);
    GLuint readHead = counter._sizeFactor - counter._writeHead - 1;
    vectorImpl<GLuint> data(counter._sizeFactor, 0);
    glNamedBufferSubData(counter._handle, readHead * sizeof(GLuint), sizeof(GLuint), data.data());
}

void glUniformBuffer::printInfo(const ShaderProgram* shaderProgram,
                                U32 bindIndex) {
    GLuint prog = shaderProgram->getID();
    GLuint block_index = bindIndex;

    if (prog <= 0 || _unbound) {
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
    for (vectorImpl<stringImpl>::iterator detail = std::begin(uniform_details);
         detail != std::end(uniform_details); ++detail) {
        Console::printfn("%s", (*detail).c_str());
    }
}

};  // namespace Divide
