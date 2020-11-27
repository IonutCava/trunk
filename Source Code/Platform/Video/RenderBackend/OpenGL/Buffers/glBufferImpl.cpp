#include "stdafx.h"

#include "Headers/glBufferImpl.h"
#include "Headers/glBufferLockManager.h"
#include "Headers/glMemoryManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {
namespace {
    SharedMutex g_CreateBufferLock;

    constexpr size_t g_MinZeroDataSize = 1 * 1024; //1Kb
    constexpr size_t g_persistentMapSizeThreshold = 512 * 1024; //512Kb
    constexpr I32 g_maxFlushQueueLength = 16;

    vectorEASTL<Byte> g_zeroMemData(g_MinZeroDataSize, Byte{ 0 });

    struct BindConfigEntry {
        U32 _handle = 0;
        size_t _offset = 0;
        size_t _range = 0;
    };

    using BindConfig = std::array<BindConfigEntry, to_base(ShaderBufferLocation::COUNT)>;
    BindConfig g_currentBindConfig;

    bool SetIfDifferentBindRange(const U32 UBOid,
                                 const U32 bindIndex,
                                 const size_t offsetInBytes,
                                 const size_t rangeInBytes) noexcept {

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
      _elementSize(params._elementSize),
      _alignedSize(params._dataSize),
      _target(params._target),
      _unsynced(params._unsynced),
      _useExplicitFlush(_target == GL_ATOMIC_COUNTER_BUFFER ? false : params._explicitFlush),
      _updateFrequency(params._frequency),
      _updateUsage(params._updateUsage),
      _context(context)
{

    std::atomic_init(&_flushQueueSize, 0);

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

    // Initial data may not fill the entire buffer
     const bool needsAdditionalData = params._initialData.second < _alignedSize;

     if (needsAdditionalData) {
         bool resizeVec;
         // If the buffer is larger than our initial data, we need to create a zero filled container large enough for proper initialization
         {
            SharedLock<SharedMutex> r_lock(g_CreateBufferLock);
            resizeVec = _alignedSize > g_zeroMemData.size();
         }

         if (resizeVec) {
             UniqueLock<SharedMutex> w_lock(g_CreateBufferLock);
             if (_alignedSize > g_zeroMemData.size()) {
                 g_zeroMemData.resize(_alignedSize, Byte{ 0 });
             }
         }
     }
    // Create all buffers with zero mem and then write the actual data that we have (If we want to initialise all memory)
    if (!usePersistentMapping) {
        if (needsAdditionalData) {
            // This should be safe as we are only INCREASING the zeroMemData container size if needed so the range we need should be fine
            SharedLock<SharedMutex> r_lock(g_CreateBufferLock);
             GLUtil::createAndAllocBuffer(_alignedSize, _usage, _bufferID, g_zeroMemData.data(), params._name);
        } else {
            GLUtil::createAndAllocBuffer(_alignedSize, _usage, _bufferID, params._initialData.first, params._name);
        }
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
        }

        if (needsAdditionalData) {
            // This should be safe as we are only INCREASING the zeroMemData container size if needed so the range we need should be fine
            SharedLock<SharedMutex> r_lock(g_CreateBufferLock);
            _mappedBuffer = (Byte*)GLUtil::createAndAllocPersistentBuffer(_alignedSize, storageMask, accessMask, _bufferID, g_zeroMemData.data(), params._name);
        } else {
            _mappedBuffer = (Byte*)GLUtil::createAndAllocPersistentBuffer(_alignedSize, storageMask, accessMask, _bufferID, params._initialData.first, params._name);
        }

        assert(_mappedBuffer != nullptr && "PersistentBuffer::Create error: Can't mapped persistent buffer!");
        if (!_unsynced) {
            _lockManager = MemoryManager_NEW glBufferLockManager();
        }
    }

    // Write our initial data now
    if (needsAdditionalData && params._initialData.second > 0) {
        writeData(0, params._initialData.second, params._initialData.first);
    }
}

glBufferImpl::~glBufferImpl()
{
    if (bufferID() > 0) {
        if (!waitRange(0, _alignedSize, false)) {
            DIVIDE_UNEXPECTED_CALL();
        }
        if (_mappedBuffer == nullptr) {
            glInvalidateBufferData(_bufferID);
        }

        GLUtil::freeBuffer(_bufferID, _mappedBuffer);
    }

    MemoryManager::SAFE_DELETE(_lockManager);
}

bool glBufferImpl::waitRange(const size_t offsetInBytes, const size_t rangeInBytes, const bool blockClient) const {
    OPTICK_EVENT();

    if (_lockManager) {
        return _lockManager->WaitForLockedRange(offsetInBytes, rangeInBytes, blockClient);
    }

    return true;
}

bool glBufferImpl::lockRange(const size_t offsetInBytes, const size_t rangeInBytes, const U32 frameID) const {
    OPTICK_EVENT();

    if (_lockManager) {
        return _lockManager->LockRange(offsetInBytes, rangeInBytes, frameID);
    }

    return true;
}

bool glBufferImpl::bindRange(const GLuint bindIndex, const size_t offsetInBytes, const size_t rangeInBytes) {
    assert(bufferID() != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool bound = false;
    if (bindIndex == to_base(ShaderBufferLocation::CMD_BUFFER)) {
        GLStateTracker& stateTracker = GL_API::getStateTracker();
        const GLuint newOffset = static_cast<GLuint>(offsetInBytes / sizeof(IndirectDrawCommand));

        bound = stateTracker.setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, bufferID());
        if (stateTracker._commandBufferOffset != newOffset) {
            stateTracker._commandBufferOffset = newOffset;
            bound = true;
        }
    } else if (SetIfDifferentBindRange(bufferID(), bindIndex, offsetInBytes, rangeInBytes)) {
        glBindBufferRange(_target, bindIndex, bufferID(), offsetInBytes, rangeInBytes);
        bound = true;
    }

    if (_useExplicitFlush) {
        size_t minOffset = std::numeric_limits<size_t>::max();
        size_t maxRange = 0;

        BufferMapRange range;
        while (_flushQueue.try_dequeue(range)) {
            minOffset = std::min(minOffset, range._offset);
            maxRange = std::max(maxRange, range._range);
            _flushQueueSize.fetch_sub(1);
        }
        if (minOffset < maxRange) {
            glFlushMappedNamedBufferRange(bufferID(), minOffset, maxRange);
        }
    }

    return bound;
}

void glBufferImpl::zeroMem(const size_t offsetInBytes, const size_t rangeInBytes) {
    if (_mappedBuffer) {
        writeOrClearData(offsetInBytes, rangeInBytes, nullptr, true);
        return;
    }

    const size_t targetSize = rangeInBytes - offsetInBytes;

    {
        SharedLock<SharedMutex> r_lock(g_CreateBufferLock);
        if (targetSize <= g_zeroMemData.size()) {
            writeOrClearData(offsetInBytes, rangeInBytes, g_zeroMemData.data(), true);
            return;
        }
    }

    UniqueLock<SharedMutex> w_lock(g_CreateBufferLock);
    // Check again to prevent race conditions from screwing this up
    if (targetSize > g_zeroMemData.size()) {
        g_zeroMemData.resize(targetSize, Byte{ 0 });
    }

    writeOrClearData(offsetInBytes, rangeInBytes, g_zeroMemData.data(), true);
}

void glBufferImpl::writeData(const size_t offsetInBytes, const size_t rangeInBytes, const Byte* data) {
    writeOrClearData(offsetInBytes, rangeInBytes, data, false);
}

void glBufferImpl::writeOrClearData(const size_t offsetInBytes, const size_t rangeInBytes, const Byte * data, const bool zeroMem) {
    OPTICK_EVENT();
    OPTICK_TAG("Mapped", static_cast<bool>(_mappedBuffer != nullptr));
    OPTICK_TAG("Offset", to_U32(offsetInBytes));
    OPTICK_TAG("Range", to_U32(rangeInBytes));
    assert(rangeInBytes > 0);

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);

    if (_mappedBuffer) {
        if (waitOK) {
            const bufferPtr dest = _mappedBuffer + offsetInBytes;
            if (zeroMem) {
                std::memset(dest, 0, rangeInBytes);
            } else {
                std::memcpy(dest, data, rangeInBytes);
            }

            if (_useExplicitFlush) {
                if (_flushQueue.enqueue(BufferMapRange{offsetInBytes, rangeInBytes})) {
                    const I32 size = _flushQueueSize.fetch_add(1);
                    if (size >= g_maxFlushQueueLength) {
                        BufferMapRange temp;
                        _flushQueue.try_dequeue(temp);
                    }
                }
            }
        } else {
            DIVIDE_UNEXPECTED_CALL();
        }
    } else {
        if (offsetInBytes == 0 && rangeInBytes == _alignedSize) {
            glInvalidateBufferData(bufferID());
            glNamedBufferData(bufferID(), _alignedSize, data, _usage);
        } else {
            glInvalidateBufferSubData(bufferID(), offsetInBytes, rangeInBytes);
            glNamedBufferSubData(bufferID(), offsetInBytes, rangeInBytes, data);
        }
    }
}

void glBufferImpl::readData(const size_t offsetInBytes, const size_t rangeInBytes, Byte* data) const {

    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        glMemoryBarrier(MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT);
    }

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);
    if (_mappedBuffer != nullptr) {
        if (waitOK) {
            if (_target != GL_ATOMIC_COUNTER_BUFFER) {
                glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            }
            const Byte* src = (Byte*)_mappedBuffer+offsetInBytes;
            std::memcpy(data, src, rangeInBytes);
        } else { 
            DIVIDE_UNEXPECTED_CALL();
        }

    } else {
        void* bufferData = glMapNamedBufferRange(bufferID(), offsetInBytes, rangeInBytes, MapBufferAccessMask::GL_MAP_READ_BIT);
        if (bufferData != nullptr) {
            std::memcpy(data, (Byte*)bufferData+offsetInBytes, rangeInBytes);
        }
        glUnmapNamedBuffer(bufferID());
    }
}

GLenum glBufferImpl::GetBufferUsage(const BufferUpdateFrequency frequency, const BufferUpdateUsage usage) noexcept {
    switch (frequency) {
        case BufferUpdateFrequency::ONCE:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_STATIC_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_STATIC_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_STATIC_COPY;
                default: break;
            };
            break;
        case BufferUpdateFrequency::OCASSIONAL:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_DYNAMIC_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_DYNAMIC_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_DYNAMIC_COPY;
                default: break;
            };
            break;
        case BufferUpdateFrequency::OFTEN:
            switch (usage) {
                case BufferUpdateUsage::CPU_W_GPU_R: return GL_STREAM_DRAW;
                case BufferUpdateUsage::CPU_R_GPU_W: return GL_STREAM_READ;
                case BufferUpdateUsage::GPU_R_GPU_W: return GL_STREAM_COPY;
                default: break;
            };
            break;
        default: break;
    };

    DIVIDE_UNEXPECTED_CALL();
    return GL_NONE;
}

void glBufferImpl::CleanMemory() noexcept {
    {
        SharedLock<SharedMutex> r_lock(g_CreateBufferLock);
        const size_t crtSize = g_zeroMemData.size();
        if (crtSize <= g_MinZeroDataSize) {
            return;
        }
    }

    UniqueLock<SharedMutex> w_lock(g_CreateBufferLock);
    const size_t crtSize = g_zeroMemData.size();
    // Speed things along if we are in the megabyte range. Not really needed, but still helps.
    const U8 reduceFactor = crtSize > g_MinZeroDataSize * 100 ? 4 : 2;
    
    g_zeroMemData.resize(std::max(g_zeroMemData.size() / reduceFactor, g_MinZeroDataSize));
}

}; //namespace Divide