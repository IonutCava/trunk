#include "Headers/glBufferImpl.h"
#include "Headers/glMemoryManager.h"
#include "Headers/glBufferLockManager.h"

namespace Divide {
namespace {

    typedef std::array<vec3<size_t>, to_const_uint(ShaderBufferLocation::COUNT)> BindConfig;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
                                 U32 bindIndex,
                                 size_t offset,
                                 size_t range) {

        vec3<size_t>& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig.x != static_cast<size_t>(UBOid) ||
            crtConfig.y != offset ||
            crtConfig.z != range) {
            crtConfig.set(static_cast<size_t>(UBOid), offset, range);
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

bool glBufferImpl::bindRange(GLuint bindIndex, size_t offset, size_t range) {
    DIVIDE_ASSERT(_UBOid != 0, "BufferImpl error: Tried to bind an uninitialized UBO");

    bool success = false;
    if (setIfDifferentBindRange(_UBOid, bindIndex, offset, range))
    {
        glBindBufferRange(_target, bindIndex, _UBOid, offset, range);
        success = true;
    }

    return success;
}

void glBufferImpl::lockRange(size_t offset, size_t range) {
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

void glRegularBuffer::updateData(size_t offset, size_t range, const bufferPtr data)
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

void glPersistentBuffer::updateData(size_t offset, size_t range, const bufferPtr data)
{
    DIVIDE_ASSERT(_mappedBuffer != nullptr, "PersistentBuffer::UpdateData error: was called for an unmapped buffer!");
    _lockManager->WaitForLockedRange(offset, range, true);
    memcpy((U8*)(_mappedBuffer)+offset, data, range);
}

bool glPersistentBuffer::bindRange(GLuint bindIndex, size_t offset, size_t range)
{
    if (glBufferImpl::bindRange(bindIndex, offset, range)) {
        _lockManager->LockRange(offset, range, false);
        return true;
    }

    return false;
}

void glPersistentBuffer::lockRange(size_t offset, size_t range) {
    _lockManager->LockRange(offset, range, true);
}

}; //namespace Divide