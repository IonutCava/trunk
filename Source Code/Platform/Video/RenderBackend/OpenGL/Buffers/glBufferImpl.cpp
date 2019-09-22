#include "stdafx.h"

#include "Headers/glBufferImpl.h"
#include "Headers/glMemoryManager.h"
#include "Headers/glBufferLockManager.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {
namespace {

    const size_t g_persistentMapSizeThreshold = 512 * 1024; //512Kb
    struct BindConfigEntry {
        U32 _handle = 0;
        GLintptr _offset = 0;
        GLsizeiptr _range = 0;
    };

    typedef std::array<BindConfigEntry, to_base(ShaderBufferLocation::COUNT)> BindConfig;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
                                 U32 bindIndex,
                                 GLintptr offsetInBytes,
                                 GLsizeiptr rangeInBytes) {

        BindConfigEntry& crtConfig = g_currentBindConfig[bindIndex];

        if (crtConfig._handle != UBOid ||
            crtConfig._offset != offsetInBytes ||
            crtConfig._range != rangeInBytes)
        {
            crtConfig = { UBOid, offsetInBytes, rangeInBytes };
            return true;
        }

        return false;
    }
};

glBufferImpl::glBufferImpl(GFXDevice& context, const BufferImplParams& params)
    : glObject(glObjectType::TYPE_BUFFER, context),
      GUIDWrapper(),
      _context(context),
      _alignedSize(params._dataSize),
      _target(params._target),
      _unsynced(params._unsynced),
      _useExplicitFlush(_target == GL_ATOMIC_COUNTER_BUFFER ? false : params._explicitFlush),
      _updateFrequency(params._frequency),
      _elementSize(params._elementSize)
{
    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        _usage = GL_STREAM_READ;
    } else {
        _usage = _updateFrequency == BufferUpdateFrequency::ONCE
                                   ? GL_STATIC_DRAW
                                   : _updateFrequency == BufferUpdateFrequency::OCASSIONAL
                                                       ? GL_DYNAMIC_DRAW
                                                       : GL_STREAM_DRAW;
    }

    bool usePersistentMapping = false;

    if (!Config::Profile::DISABLE_PERSISTENT_BUFFER) {  // For debugging
        if (params._storageType == BufferStorageType::IMMUTABLE) {
            usePersistentMapping = true;
        } else if (params._storageType == BufferStorageType::AUTO) {
            usePersistentMapping = _alignedSize > g_persistentMapSizeThreshold;    // Driver might be faster?
        }
    }

    // Why do we need to map it?
    if (_updateFrequency == BufferUpdateFrequency::ONCE) {
        usePersistentMapping = false;
    }

    if (!usePersistentMapping) {
        GLUtil::createAndAllocBuffer(_alignedSize, _usage, _handle, params._initialData, params._name);
    } else {
        gl::BufferStorageMask storageMask = GL_MAP_PERSISTENT_BIT;
        gl::BufferAccessMask accessMask = GL_MAP_PERSISTENT_BIT;

        switch (_updateFrequency) {
            case BufferUpdateFrequency::ONCE:{
                storageMask |= GL_MAP_READ_BIT;
                accessMask |= GL_MAP_READ_BIT;
            } break;
            case BufferUpdateFrequency::OCASSIONAL:
            case BufferUpdateFrequency::OFTEN: {
                storageMask |= GL_MAP_WRITE_BIT;
                accessMask |= GL_MAP_WRITE_BIT;
                
                if (_useExplicitFlush) {
                    accessMask |= GL_MAP_FLUSH_EXPLICIT_BIT;
                }
          
            } break;
            case BufferUpdateFrequency::COUNT: {
                DIVIDE_UNEXPECTED_CALL("Unknown buffer update frequency!");
            } break;
        };
  
        _mappedBuffer = GLUtil::createAndAllocPersistentBuffer(_alignedSize, storageMask, accessMask, _handle, params._initialData, params._name);

        assert(_mappedBuffer != nullptr && "PersistentBuffer::Create error: Can't mapped persistent buffer!");
    }

    if (params._zeroMem) {
        zeroMem(0, _alignedSize);
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
}

bool glBufferImpl::waitRange(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, bool blockClient) {
    if (_mappedBuffer != nullptr && !_unsynced) {
        //assert(!GL_API::s_glFlushQueued);

        return GL_API::getLockManager().WaitForLockedRange(bufferID(), offsetInBytes, rangeInBytes);
    }

    return true;
}

void glBufferImpl::lockRange(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, bool flush) {
    if (_mappedBuffer != nullptr && !_unsynced) {
        BufferWriteData data = {};
        data._handle = bufferID();
        data._offset = offsetInBytes;
        data._range = rangeInBytes;
        data._flush = flush;

        GL_API::registerBufferBind(data);
    }
}

GLuint glBufferImpl::bufferID() const {
    return _handle;
}

bool glBufferImpl::bindRange(GLuint bindIndex, GLintptr offsetInBytes, GLsizeiptr rangeInBytes) {
    assert(_handle != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool wasBound = true;
    if (setIfDifferentBindRange(_handle, bindIndex, offsetInBytes, rangeInBytes))
    {
        glBindBufferRange(_target, bindIndex, _handle, offsetInBytes, rangeInBytes);
        wasBound = false;
    }

    return !wasBound;
}

void glBufferImpl::writeData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, const bufferPtr data)
{
    if (_mappedBuffer) {
        waitRange(offsetInBytes, rangeInBytes, true);

        std::memcpy(((Byte*)_mappedBuffer) + offsetInBytes, data, rangeInBytes);
        if (_useExplicitFlush) {
            glFlushMappedNamedBufferRange(_handle, offsetInBytes, rangeInBytes);
        }
    } else {
        clearData(offsetInBytes, rangeInBytes);
        if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
            glNamedBufferData(_handle, _alignedSize, data, _usage);
        } else {
            glNamedBufferSubData(_handle, offsetInBytes, rangeInBytes, data);
        }
    }
}

void glBufferImpl::readData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes, const bufferPtr data)
{
    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        glMemoryBarrier(MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT);
    }

    if (_mappedBuffer && waitRange(offsetInBytes, rangeInBytes, true)) {
        if (_target != GL_ATOMIC_COUNTER_BUFFER) {
            glMemoryBarrier(MemoryBarrierMask::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
        }
        std::memcpy(data, ((Byte*)(_mappedBuffer)+offsetInBytes), rangeInBytes);
    } else {
        void* bufferData = glMapNamedBufferRange(_handle, offsetInBytes, rangeInBytes, BufferAccessMask::GL_MAP_READ_BIT);
        if (bufferData != nullptr) {
            std::memcpy(data, ((Byte*)(bufferData)+offsetInBytes), rangeInBytes);
        }
        glUnmapNamedBuffer(_handle);
    }
}

void glBufferImpl::clearData(GLintptr offsetInBytes, GLsizeiptr rangeInBytes) {
    if (_mappedBuffer) {
        if (!waitRange(offsetInBytes, rangeInBytes, true)) {
            //ToDo: wait failed. Now what?
        }
        std::memset(((Byte*)_mappedBuffer) + offsetInBytes, 0, rangeInBytes);
        if (_useExplicitFlush) {
            glFlushMappedNamedBufferRange(_handle, offsetInBytes, rangeInBytes);
        }
    } else {
        if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
            glInvalidateBufferData(_handle);
        } else {
            glInvalidateBufferSubData(_handle, offsetInBytes, rangeInBytes);
        }
    }
}

void glBufferImpl::zeroMem(GLintptr offsetInBytes, GLsizeiptr rangeInBytes) {
    if (_mappedBuffer) {
        clearData(offsetInBytes, rangeInBytes);
    } else {
        vector<Byte> newData(rangeInBytes, 0);
        //if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
        //    glNamedBufferData(_handle, _alignedSize, newData.data(), _usage);
        //} else
		{
            glNamedBufferSubData(_handle, offsetInBytes, rangeInBytes, newData.data());
        }
    }
}

size_t glBufferImpl::elementSize() const {
    return _elementSize;
}

}; //namespace Divide