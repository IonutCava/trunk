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
                                 size_t offsetInBytes,
                                 size_t rangeInBytes) {

        vec3<size_t>& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig.x != static_cast<size_t>(UBOid) ||
            crtConfig.y != offsetInBytes ||
            crtConfig.z != rangeInBytes)
        {
            crtConfig.set(static_cast<size_t>(UBOid), offsetInBytes, rangeInBytes);
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
    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        _usage = GL_STATIC_COPY;
    } else {
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
    }

    bool usePersistentMapping = !Config::Profile::DISABLE_PERSISTENT_BUFFER &&  // For debugging
                                !Config::USE_THREADED_COMMAND_GENERATION &&     // For ease-of-use
                                _alignedSize > g_persistentMapSizeThreshold;    // Driver might be faster?

    if (!usePersistentMapping) {
        GLUtil::createAndAllocBuffer(_alignedSize, _usage, _handle, params._initialData, params._name);
    } else {
        gl::BufferStorageMask storageMask = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        if (_updateFrequency != BufferUpdateFrequency::ONCE) {
            storageMask |= GL_DYNAMIC_STORAGE_BIT;
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

void glBufferImpl::waitRange(size_t offsetInBytes, size_t rangeInBytes, bool blockClient) {
    _lockManager->WaitForLockedRange(offsetInBytes, rangeInBytes, blockClient);
}

void glBufferImpl::lockRange(size_t offsetInBytes, size_t rangeInBytes) {
    _lockManager->LockRange(offsetInBytes, rangeInBytes);
}

GLuint glBufferImpl::bufferID() const {
    return _handle;
}

bool glBufferImpl::bindRange(GLuint bindIndex, size_t offsetInBytes, size_t rangeInBytes) {
    assert(_handle != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool wasBound = true;
    if (setIfDifferentBindRange(_handle, bindIndex, offsetInBytes, rangeInBytes))
    {
        glBindBufferRange(_target, bindIndex, _handle, offsetInBytes, rangeInBytes);
        wasBound = false;
    }
    if (_mappedBuffer) {
        lockRange(offsetInBytes, rangeInBytes);
    }

    return !wasBound;
}

void glBufferImpl::writeData(size_t offsetInBytes, size_t rangeInBytes, const bufferPtr data)
{
    if (_mappedBuffer) {
        waitRange(offsetInBytes, rangeInBytes, true);
        std::memcpy(((Byte*)_mappedBuffer) + offsetInBytes,
                     data,
                     rangeInBytes);
    } else {
        if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
            glInvalidateBufferData(_handle);
            glNamedBufferData(_handle, _alignedSize, data, _usage);
        } else {
            glInvalidateBufferSubData(_handle, offsetInBytes, rangeInBytes);
            glNamedBufferSubData(_handle, offsetInBytes, rangeInBytes, data);
        }
    }
}

void glBufferImpl::readData(size_t offsetInBytes, size_t rangeInBytes, const bufferPtr data)
{
    if (_mappedBuffer) {
        memcpy(data, ((Byte*)(_mappedBuffer)+offsetInBytes), rangeInBytes);
    } else {
        glGetNamedBufferSubData(_handle, offsetInBytes, rangeInBytes, data);
    }
}

}; //namespace Divide