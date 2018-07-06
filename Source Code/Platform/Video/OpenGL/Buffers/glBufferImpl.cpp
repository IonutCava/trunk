#include "Headers/glBufferImpl.h"
#include "Headers/glMemoryManager.h"
#include "Headers/glBufferLockManager.h"

namespace Divide {
namespace {

    typedef std::array<vec3<U32>, to_const_uint(ShaderBufferLocation::COUNT)> BindConfig;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
        U32 bindIndex,
        U32 offset,
        U32 range) {

        vec3<U32>& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig.x != UBOid ||
            crtConfig.y != offset ||
            crtConfig.z != range) {
            crtConfig.set(UBOid, offset, range);
            return true;
        }

        return false;
    }
};

glBufferImpl::glBufferImpl(GLenum target) 
    : _target(target),
      _UBOid(0),
      _alignedSize(0)
{
}

glBufferImpl::~glBufferImpl()
{
    assert(_UBOid == 0);
}

GLuint glBufferImpl::bufferID() const {
    return _UBOid;
}

void glBufferImpl::create(BufferUpdateFrequency frequency, size_t size)
{
    DIVIDE_ASSERT(_UBOid == 0, "BufferImpl::Create error: Tried to double create current UBO");
}

bool glBufferImpl::bindRange(GLuint bindIndex, GLuint offset, GLuint range) {
    DIVIDE_ASSERT(_UBOid != 0, "BufferImpl error: Tried to bind an uninitialized UBO");

    bool success = false;
    if (setIfDifferentBindRange(_UBOid, bindIndex, offset, range))
    {
        glBindBufferRange(_target, bindIndex, _UBOid, offset, range);
        success = true;
    }

    return success;
}

void glBufferImpl::lockRange(GLuint offset, GLuint range) {
}

glRegularBuffer::glRegularBuffer(GLenum target)
    : glBufferImpl(target)
{
}

glRegularBuffer::~glRegularBuffer()
{
}

void glRegularBuffer::create(BufferUpdateFrequency frequency, size_t size)
{
    glBufferImpl::create(frequency, size);
    GLenum usage =
        _target == GL_TRANSFORM_FEEDBACK
                 ? frequency == BufferUpdateFrequency::ONCE
                              ? GL_STATIC_COPY
                              : frequency == BufferUpdateFrequency::OCASSIONAL
                                           ? GL_DYNAMIC_COPY
                                           : GL_STREAM_COPY
                  : frequency == BufferUpdateFrequency::ONCE
                               ? GL_STATIC_DRAW
                               : frequency == BufferUpdateFrequency::OCASSIONAL
                                            ? GL_DYNAMIC_DRAW
                                            : GL_STREAM_DRAW;
    GLUtil::createAndAllocBuffer(size, usage, _UBOid);
}

void glRegularBuffer::destroy()
{
    if (_UBOid > 0) {
        glInvalidateBufferData(_UBOid);
        GLUtil::freeBuffer(_UBOid, nullptr);
    }
}

void glRegularBuffer::updateData(GLintptr offset, GLintptr range, const bufferPtr data)
{
    //glInvalidateBufferSubData(_UBOid, offset, range);
    glNamedBufferSubData(_UBOid, offset, range, data);
}

glPersistentBuffer::glPersistentBuffer(GLenum target) 
    : glBufferImpl(target),
      _mappedBuffer(nullptr),
      _lockManager(MemoryManager_NEW glBufferLockManager())
{
}

glPersistentBuffer::~glPersistentBuffer()
{
    MemoryManager::DELETE(_lockManager);
}

void glPersistentBuffer::create(BufferUpdateFrequency frequency, size_t size)
{
    glBufferImpl::create(frequency, size);
    _mappedBuffer =
        GLUtil::createAndAllocPersistentBuffer(size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT,
            _UBOid);

    DIVIDE_ASSERT(_mappedBuffer != nullptr, "PersistentBuffer::Create error: Can't mapped persistent buffer!");
}

void glPersistentBuffer::destroy()
{
    if (_UBOid > 0) {
        _lockManager->WaitForLockedRange(0, _alignedSize, false);
        GLUtil::freeBuffer(_UBOid, _mappedBuffer);
    }
}

void glPersistentBuffer::updateData(GLintptr offset, GLintptr range, const bufferPtr data)
{
    DIVIDE_ASSERT(_mappedBuffer != nullptr, "PersistentBuffer::UpdateData error: was called for an unmapped buffer!");
    _lockManager->WaitForLockedRange(offset, range, true);
    memcpy((U8*)(_mappedBuffer)+offset, data, range);
}

bool glPersistentBuffer::bindRange(GLuint bindIndex, GLuint offset, GLuint range)
{
    if (glBufferImpl::bindRange(bindIndex, offset, range)) {
        _lockManager->LockRange(offset, range, false);
        return true;
    }

    return false;
}

void glPersistentBuffer::lockRange(GLuint offset, GLuint range) {
    _lockManager->LockRange(offset, range, true);
}

}; //namespace Divide