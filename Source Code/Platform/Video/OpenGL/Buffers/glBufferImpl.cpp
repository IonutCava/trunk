#include "stdafx.h"

#include "Headers/glBufferImpl.h"
#include "Headers/glMemoryManager.h"
#include "Headers/glBufferLockManager.h"

namespace Divide {
namespace {

    const size_t g_persistentMapSizeThreshold = 512 * 1024; //512Kb

    typedef std::array<vec3<size_t>, to_base(ShaderBufferLocation::COUNT)> BindConfig;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
                                 U32 bindIndex,
                                 size_t offset,
                                 size_t range) {

        vec3<size_t>& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig.x != static_cast<size_t>(UBOid) ||
            crtConfig.y != offset ||
            crtConfig.z != range)
        {
            crtConfig.set(static_cast<size_t>(UBOid), offset, range);
            return true;
        }

        return false;
    }
};

glBufferImpl::glBufferImpl(GFXDevice& context, const BufferImplParams& params)
    : glObject(glObjectType::TYPE_BUFFER, context),
      _target(params._target),
      _handle(0),
      _alignedSize(params._dataSizeInBytes),
      _updateFrequency(params._frequency),
      _mappedBuffer(nullptr),
      _lockManager(MemoryManager_NEW glBufferLockManager())
{

    _usage = _target == GL_TRANSFORM_FEEDBACK
                        ? _updateFrequency == BufferUpdateFrequency::ONCE
                                            ? GL_STATIC_COPY
                                            : _updateFrequency == BufferUpdateFrequency::OCASSIONAL
                                                                ? GL_DYNAMIC_COPY
                                                                : GL_STREAM_COPY
                        : _updateFrequency == BufferUpdateFrequency::ONCE
                                            ? GL_STATIC_DRAW
                                            : _updateFrequency == BufferUpdateFrequency::OCASSIONAL
                                                                ? GL_DYNAMIC_DRAW
                                                                : GL_STREAM_DRAW;

    bool usePersistentMapping = !Config::Profile::DISABLE_PERSISTENT_BUFFER && 
                                _alignedSize > g_persistentMapSizeThreshold;

    if (!usePersistentMapping) {
        GLUtil::createAndAllocBuffer(_alignedSize, _usage, _handle, params._initialData, params._name);
    } else {
        gl::BufferStorageMask storageMask = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        if (_updateFrequency != BufferUpdateFrequency::ONCE) {
            //storageMask |= GL_DYNAMIC_STORAGE_BIT;
        }

        gl::BufferAccessMask accessMask = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

        _mappedBuffer = GLUtil::createAndAllocPersistentBuffer(_alignedSize, storageMask, accessMask, _handle, params._initialData, params._name);

        assert(_mappedBuffer != nullptr && "PersistentBuffer::Create error: Can't mapped persistent buffer!");
    }
}

glBufferImpl::~glBufferImpl()
{
    if (_handle > 0) {
        waitRange(0, _alignedSize, false);
        if (_mappedBuffer == nullptr) {
            glInvalidateBufferData(_handle);
        }

        GLUtil::freeBuffer(_handle, _mappedBuffer);
    }

    MemoryManager::DELETE(_lockManager);
}

void glBufferImpl::waitRange(size_t offset, size_t range, bool blockClient) {
    _lockManager->WaitForLockedRange(offset, range, blockClient);
}

void glBufferImpl::lockRange(size_t offset, size_t range) {
    _lockManager->LockRange(offset, range);
}

GLuint glBufferImpl::bufferID() const {
    return _handle;
}

bool glBufferImpl::bindRange(GLuint bindIndex, size_t offset, size_t range) {
    assert(_handle != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool wasBound = true;
    if (setIfDifferentBindRange(_handle, bindIndex, offset, range))
    {
        glBindBufferRange(_target, bindIndex, _handle, offset, range);
        wasBound = false;
    }
    if (_mappedBuffer) {
        lockRange(offset, range);
    }

    return !wasBound;
}

void glBufferImpl::writeData(size_t offset, size_t range, const bufferPtr data)
{
    if (_mappedBuffer) {
        waitRange(offset, range, true);
        assert(_mappedBuffer != nullptr && "PersistentBuffer::UpdateData error: was called for an unmapped buffer!");
        if (data) {
            memcpy(((U8*)_mappedBuffer) + offset, data, range);
        } else {
            memset(((U8*)_mappedBuffer) + offset, 0, range);
        }
    } else {
        if (offset == 0 && range == _alignedSize) {
            glInvalidateBufferData(_handle);
            glNamedBufferData(_handle, _alignedSize, data, _usage);
        } else {
            glInvalidateBufferSubData(_handle, offset, range);
            glNamedBufferSubData(_handle, offset, range, data);
        }
    }
}

void glBufferImpl::readData(size_t offset, size_t range, const bufferPtr data)
{
    if (_mappedBuffer) {
        memcpy(data, ((U8*)(_mappedBuffer)+offset), range);
    } else {
        glGetNamedBufferSubData(_handle, offset, range, data);
    }
}

}; //namespace Divide