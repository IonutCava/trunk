#include "stdafx.h"

#include "Headers/glBufferImpl.h"
#include "Headers/glMemoryManager.h"
#include "Headers/glBufferLockManager.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {
namespace {

    constexpr bool g_issueMemBarrierForCoherentBuffers = false;
    constexpr size_t g_persistentMapSizeThreshold = 512 * 1024; //512Kb

    struct BindConfigEntry {
        U32 _handle = 0;
        size_t _offset = 0;
        size_t _range = 0;
    };

    using BindConfig = std::array<BindConfigEntry, to_base(ShaderBufferLocation::COUNT)>;
    BindConfig g_currentBindConfig;

    bool setIfDifferentBindRange(U32 UBOid,
                                 U32 bindIndex,
                                 size_t offsetInBytes,
                                 size_t rangeInBytes) noexcept {

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
      _updateUsage(params._updateUsage),
      _elementSize(params._elementSize),
      _lockManager(nullptr)
{
    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        _usage = GL_STREAM_READ;
    } else {
        _usage = GetBufferUsage(_updateFrequency, _updateUsage);
    }

    bool usePersistentMapping = false;

    if (params._storageType == BufferStorageType::IMMUTABLE) {
        usePersistentMapping = true;
    } else if (params._storageType == BufferStorageType::AUTO) {
        usePersistentMapping = _alignedSize > g_persistentMapSizeThreshold;    // Driver might be faster?
    }

    // Why do we need to map it?
    if (_updateFrequency == BufferUpdateFrequency::ONCE || _updateUsage == BufferUpdateUsage::GPU_R_GPU_W) {
        usePersistentMapping = false;
    }

    if (!usePersistentMapping) {
        GLUtil::createAndAllocBuffer(_alignedSize, _usage, _handle, params._initialData, params._name);
    } else {
        BufferStorageMask storageMask = GL_MAP_PERSISTENT_BIT;
        MapBufferAccessMask accessMask = GL_MAP_PERSISTENT_BIT;

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
                } else {
                    storageMask |= GL_MAP_COHERENT_BIT;
                    accessMask |= GL_MAP_COHERENT_BIT;
                }
          
            } break;
            case BufferUpdateFrequency::COUNT: {
                DIVIDE_UNEXPECTED_CALL("Unknown buffer update frequency!");
            } break;
        };
  
        _mappedBuffer = (Byte*)GLUtil::createAndAllocPersistentBuffer(_alignedSize, storageMask, accessMask, _handle, params._initialData, params._name);

        assert(_mappedBuffer != nullptr && "PersistentBuffer::Create error: Can't mapped persistent buffer!");
    }

    if (_mappedBuffer != nullptr && !_unsynced) {
        _lockManager = MemoryManager_NEW glBufferLockManager();
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

    MemoryManager::SAFE_DELETE(_lockManager);
}

bool glBufferImpl::waitRange(size_t offsetInBytes, size_t rangeInBytes, bool blockClient) {
    OPTICK_EVENT();

    if (_lockManager) {
        return _lockManager->WaitForLockedRange(offsetInBytes, rangeInBytes, blockClient);
    }

    return true;
}

void glBufferImpl::lockRange(size_t offsetInBytes, size_t rangeInBytes, U32 frameID) {
    if (_lockManager) {
        _lockManager->LockRange(offsetInBytes, rangeInBytes, frameID);
    }
}

GLuint glBufferImpl::bufferID() const noexcept {
    return _handle;
}

bool glBufferImpl::bindRange(GLuint bindIndex, size_t offsetInBytes, size_t rangeInBytes) {
    assert(_handle != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool bound = false;
    if (bindIndex == to_base(ShaderBufferLocation::CMD_BUFFER)) {
        GLStateTracker& stateTracker = GL_API::getStateTracker();
        const GLuint newOffset = static_cast<GLuint>(offsetInBytes / sizeof(IndirectDrawCommand));

        bound = stateTracker.setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, _handle);
        if (stateTracker._commandBufferOffset != newOffset) {
            stateTracker._commandBufferOffset = newOffset;
            bound = true;
        }
    } else if (setIfDifferentBindRange(_handle, bindIndex, offsetInBytes, rangeInBytes)) {
        glBindBufferRange(_target, bindIndex, _handle, offsetInBytes, rangeInBytes);
        bound = true;
    }

    return bound;
}

void glBufferImpl::writeData(size_t offsetInBytes, size_t rangeInBytes, const Byte* data)
{
    OPTICK_EVENT();
    OPTICK_TAG("Mapped", bool(_mappedBuffer != nullptr));
    OPTICK_TAG("Offset", to_U32(offsetInBytes));
    OPTICK_TAG("Range", to_U32(rangeInBytes));

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);

    invalidateData(offsetInBytes, rangeInBytes);

    if (_mappedBuffer) {
        if (waitOK) {
            const bufferPtr dest = _mappedBuffer + offsetInBytes;

            std::memcpy(dest, data, rangeInBytes);
            if (_useExplicitFlush) {
                glFlushMappedNamedBufferRange(_handle, offsetInBytes, rangeInBytes);
            } else if (g_issueMemBarrierForCoherentBuffers) {
                glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            }
        } else {
            DIVIDE_UNEXPECTED_CALL();
        }
    } else {
        if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
            glNamedBufferData(_handle, _alignedSize, data, _usage);
        } else {
            glNamedBufferSubData(_handle, offsetInBytes, rangeInBytes, data);
        }
    }
}

void glBufferImpl::readData(size_t offsetInBytes, size_t rangeInBytes, Byte* data)
{
    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        glMemoryBarrier(MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT);
    }

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);
    if (_mappedBuffer != nullptr) {
        if (waitOK) {
            if (_target != GL_ATOMIC_COUNTER_BUFFER) {
                glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            }
            const Byte* src = ((Byte*)(_mappedBuffer)+offsetInBytes);
            std::memcpy(data, src, rangeInBytes);
        } else { 
            DIVIDE_UNEXPECTED_CALL();
        }

    } else {
        void* bufferData = glMapNamedBufferRange(_handle, offsetInBytes, rangeInBytes, MapBufferAccessMask::GL_MAP_READ_BIT);
        if (bufferData != nullptr) {
            std::memcpy(data, ((Byte*)(bufferData)+offsetInBytes), rangeInBytes);
        }
        glUnmapNamedBuffer(_handle);
    }
}

void glBufferImpl::invalidateData(size_t offsetInBytes, size_t rangeInBytes) {
    OPTICK_EVENT();

    if (_mappedBuffer != nullptr) {
        return;
    }

    if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
        glInvalidateBufferData(_handle);
    } else {
        glInvalidateBufferSubData(_handle, offsetInBytes, rangeInBytes);
    }
}

void glBufferImpl::zeroMem(size_t offsetInBytes, size_t rangeInBytes) {
    const vectorSTD<Byte> newData(rangeInBytes, Byte{ 0 });
    writeData(offsetInBytes, rangeInBytes, newData.data());
}

size_t glBufferImpl::elementSize() const noexcept {
    return _elementSize;
}

GLenum glBufferImpl::GetBufferUsage(BufferUpdateFrequency frequency, BufferUpdateUsage usage) noexcept {
    switch (frequency) {
        case BufferUpdateFrequency::ONCE:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_STATIC_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_STATIC_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_STATIC_COPY;
            };
            break;
        case BufferUpdateFrequency::OCASSIONAL:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_DYNAMIC_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_DYNAMIC_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_DYNAMIC_COPY;
            };
            break;
        case BufferUpdateFrequency::OFTEN:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_STREAM_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_STREAM_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_STREAM_COPY;
            };
            break;
    };

    DIVIDE_UNEXPECTED_CALL();
    return GL_NONE;
}
}; //namespace Divide