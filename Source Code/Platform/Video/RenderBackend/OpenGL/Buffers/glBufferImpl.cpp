#include "stdafx.h"

#include "Headers/glBufferImpl.h"
#include "Headers/glBufferLockManager.h"
#include "Headers/glMemoryManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {
namespace {
    SharedMutex g_createBufferLock;

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
        usePersistentMapping = params._dataSize > g_persistentMapSizeThreshold;    // Driver might be faster?
    }

    // Why do we need to map it?
    if (_updateFrequency == BufferUpdateFrequency::ONCE || _updateUsage == BufferUpdateUsage::GPU_R_GPU_W) {
        usePersistentMapping = false;
    }

    // Initial data may not fill the entire buffer
     const bool needsAdditionalData = params._initialData.second < params._dataSize;

     if (needsAdditionalData) {
         bool resizeVec;
         // If the buffer is larger than our initial data, we need to create a zero filled container large enough for proper initialization
         {
            SharedLock<SharedMutex> r_lock(g_createBufferLock);
            resizeVec = params._dataSize > g_zeroMemData.size();
         }

         if (resizeVec) {
             UniqueLock<SharedMutex> w_lock(g_createBufferLock);
             if (params._dataSize > g_zeroMemData.size()) {
                 g_zeroMemData.resize(params._dataSize, Byte{ 0 });
             }
         }
     }
    // Create all buffers with zero mem and then write the actual data that we have (If we want to initialise all memory)
    if (!usePersistentMapping) {
        if (needsAdditionalData) {
            // This should be safe as we are only INCREASING the zeroMemData container size if needed so the range we need should be fine
            SharedLock<SharedMutex> r_lock(g_createBufferLock);
             GLUtil::createAndAllocBuffer(params._dataSize, _usage, _memoryBlock._bufferHandle, g_zeroMemData.data(), params._name);
        } else {
             GLUtil::createAndAllocBuffer(params._dataSize, _usage, _memoryBlock._bufferHandle, params._initialData.first, params._name);
        }
        _memoryBlock._offset = 0;
        _memoryBlock._size = params._dataSize;
        _memoryBlock._free = false;
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
                DIVIDE_UNEXPECTED_CALL();
            } break;
        }

        _memoryBlock = GL_API::getMemoryAllocator().allocate(params._dataSize, storageMask, accessMask, params._name);
        assert(_memoryBlock._ptr != nullptr && _memoryBlock._size == params._dataSize);
        if (needsAdditionalData) {
            SharedLock<SharedMutex> r_lock(g_createBufferLock);
            std::memcpy(_memoryBlock._ptr, g_zeroMemData.data(), _memoryBlock._size);
        } else {
            std::memcpy(_memoryBlock._ptr, params._initialData.first, _memoryBlock._size);
        }
        
        assert(_memoryBlock._ptr != nullptr && "PersistentBuffer::Create error: Can't mapped persistent buffer!");
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
    if (_memoryBlock._bufferHandle > 0) {
        if (!waitRange(0, _memoryBlock._size, false)) {
            DIVIDE_UNEXPECTED_CALL();
        }

        if (_memoryBlock._ptr != nullptr) {
            GL_API::getMemoryAllocator().deallocate(_memoryBlock);
        } else {
            glInvalidateBufferData(_memoryBlock._bufferHandle);
            GLUtil::freeBuffer(_memoryBlock._bufferHandle, nullptr);
        }
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
    assert(_memoryBlock._bufferHandle != 0 && "BufferImpl error: Tried to bind an uninitialized UBO");

    bool bound = false;
    if (bindIndex == to_base(ShaderBufferLocation::CMD_BUFFER)) {
        GL_API::getStateTracker().setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, _memoryBlock._bufferHandle);
        bound = true;
    } else if (SetIfDifferentBindRange(_memoryBlock._bufferHandle, bindIndex, offsetInBytes, rangeInBytes)) {
        glBindBufferRange(_target, bindIndex, _memoryBlock._bufferHandle, offsetInBytes, rangeInBytes);
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
            glFlushMappedNamedBufferRange(_memoryBlock._bufferHandle, minOffset, maxRange);
        }
    }

    return bound;
}

void glBufferImpl::zeroMem(const size_t offsetInBytes, const size_t rangeInBytes) {
    if ( _memoryBlock._ptr != nullptr ) {
        writeOrClearData(offsetInBytes, rangeInBytes, nullptr, true);
        return;
    }

    const size_t targetSize = rangeInBytes - offsetInBytes;

    {
        SharedLock<SharedMutex> r_lock(g_createBufferLock);
        if (targetSize <= g_zeroMemData.size()) {
            writeOrClearData(offsetInBytes, rangeInBytes, g_zeroMemData.data(), true);
            return;
        }
    }

    UniqueLock<SharedMutex> w_lock(g_createBufferLock);
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
    OPTICK_TAG("Mapped", static_cast<bool>(_memoryBlock._ptr != nullptr));
    OPTICK_TAG("Offset", to_U32(offsetInBytes));
    OPTICK_TAG("Range", to_U32(rangeInBytes));
    assert(rangeInBytes > 0);

    assert(offsetInBytes + rangeInBytes <= _memoryBlock._size);

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);

    if (_memoryBlock._ptr != nullptr) {
        if (waitOK) {
            if (zeroMem) {
                std::memset(_memoryBlock._ptr + offsetInBytes, 0, rangeInBytes);
            } else {
                std::memcpy(_memoryBlock._ptr + offsetInBytes, data, rangeInBytes);
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
        if (offsetInBytes == 0 && rangeInBytes == _memoryBlock._size) {
            glInvalidateBufferData(_memoryBlock._bufferHandle);
            glNamedBufferData(_memoryBlock._bufferHandle, _memoryBlock._size, data, _usage);
        } else {
            glInvalidateBufferSubData(_memoryBlock._bufferHandle, offsetInBytes, rangeInBytes);
            glNamedBufferSubData(_memoryBlock._bufferHandle, offsetInBytes, rangeInBytes, data);
        }
    }
}

void glBufferImpl::readData(const size_t offsetInBytes, const size_t rangeInBytes, Byte* data) const {

    if (_target == GL_ATOMIC_COUNTER_BUFFER) {
        glMemoryBarrier(MemoryBarrierMask::GL_ATOMIC_COUNTER_BARRIER_BIT);
    }

    const bool waitOK = waitRange(offsetInBytes, rangeInBytes, true);
    if (_memoryBlock._ptr != nullptr) {
        if (waitOK) {
            if (_target != GL_ATOMIC_COUNTER_BUFFER) {
                glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
            }
            std::memcpy(data, _memoryBlock._ptr + offsetInBytes, rangeInBytes);
        } else { 
            DIVIDE_UNEXPECTED_CALL();
        }

    } else {
        Byte* bufferData = (Byte*)glMapNamedBufferRange(_memoryBlock._bufferHandle, offsetInBytes, rangeInBytes, MapBufferAccessMask::GL_MAP_READ_BIT);
        if (bufferData != nullptr) {
            std::memcpy(data, bufferData + offsetInBytes, rangeInBytes);
        }
        glUnmapNamedBuffer(_memoryBlock._bufferHandle);
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
        SharedLock<SharedMutex> r_lock(g_createBufferLock);
        const size_t crtSize = g_zeroMemData.size();
        if (crtSize <= g_MinZeroDataSize) {
            return;
        }
    }

    UniqueLock<SharedMutex> w_lock(g_createBufferLock);
    const size_t crtSize = g_zeroMemData.size();
    // Speed things along if we are in the megabyte range. Not really needed, but still helps.
    const U8 reduceFactor = crtSize > g_MinZeroDataSize * 100 ? 4 : 2;
    
    g_zeroMemData.resize(std::max(g_zeroMemData.size() / reduceFactor, g_MinZeroDataSize));
}

}; //namespace Divide