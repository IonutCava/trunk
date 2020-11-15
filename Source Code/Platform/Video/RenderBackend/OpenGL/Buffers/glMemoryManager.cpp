#include "stdafx.h"

#include "Headers/glMemoryManager.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/StringHelper.h"

namespace Divide {
namespace GLUtil {


static vectorEASTL<VBO> g_globalVBOs;

U32 VBO::getChunkCountForSize(const size_t sizeInBytes) noexcept {
    return to_U32(std::ceil(to_F32(sizeInBytes) / MAX_VBO_CHUNK_SIZE_BYTES));
}

VBO::VBO()  noexcept
    : _handle(0),
      _usage(GL_NONE),
      _filledManually(false)
{
    _chunkUsageState.fill(std::make_pair(false, 0));
}

void VBO::freeAll() {
    freeBuffer(_handle);
    _usage = GL_NONE;
}

U32 VBO::handle() const noexcept {
    return _handle;
}

bool VBO::checkChunksAvailability(const size_t offset, const U32 count, U32& chunksUsedTotal) noexcept {
    assert(MAX_VBO_CHUNK_COUNT > offset);

    U32 freeChunkCount = 0;
    chunksUsedTotal = _chunkUsageState[offset].second;
    if (!_chunkUsageState[offset].first) {
        ++freeChunkCount;
        for (U32 j = 1; j < count; ++j) {
            if (_chunkUsageState[offset + j].first) {
                break;
            }

            ++freeChunkCount;
        }
    }
    
    return freeChunkCount >= count;
}

bool VBO::allocateChunks(const U32 count, const GLenum usage, size_t& offsetOut) {
    if (_usage == GL_NONE || _usage == usage) {
        U32 crtOffset = 0u;
        for (U32 i = 0; i < MAX_VBO_CHUNK_COUNT; ++i) {
            if (checkChunksAvailability(i, count, crtOffset)) {
                if (_handle == 0) {
                    createAndAllocBuffer(MAX_VBO_SIZE_BYTES, usage, _handle, nullptr, 0, true, Util::StringFormat("VBO_CHUNK_%d", i).c_str());
                    _usage = usage;
                }
                offsetOut = i;
                _chunkUsageState[i] = { true, count };
                for (U32 j = 1; j < count; ++j) {
                    _chunkUsageState[j + i].first = true;
                    _chunkUsageState[j + i].second = count - j;
                }
                return true;
            }
            if (crtOffset > 1) {
                i += crtOffset - 1;
            }
        }
    }

    return false;
}

bool VBO::allocateWhole(const U32 count, const GLenum usage) {
    static U32 idx = 0;

    assert(_handle == 0);
    createAndAllocBuffer(static_cast<size_t>(count) * MAX_VBO_CHUNK_SIZE_BYTES, usage, _handle, nullptr, 0, true, Util::StringFormat("VBO_WHOLE_CHUNK_%d", idx++).c_str());
    _usage = usage;
    _chunkUsageState.fill(std::make_pair(true, 0));
    _chunkUsageState[0].second = count;
    _filledManually = true;
    return true;
}

void VBO::releaseChunks(const size_t offset) {
    if (_filledManually) {
        _chunkUsageState.fill(std::make_pair(false, 0));
        return;
    }

    assert(offset < MAX_VBO_CHUNK_COUNT);
    assert(_chunkUsageState[offset].second != 0);

    const U32 childCount = _chunkUsageState[offset].second;
    for (size_t i = 0; i < childCount; ++i) {
        auto& [used, count] = _chunkUsageState[i + offset];
        assert(used);
        used = false;
        count = 0;
    }
}

size_t VBO::getMemUsage() noexcept {
    return MAX_VBO_CHUNK_SIZE_BYTES * 
           std::count_if(std::begin(_chunkUsageState),
                         std::end(_chunkUsageState),
                         [](const auto& it) { return it.first; });
}

bool commitVBO(const U32 chunkCount, const GLenum usage, GLuint& handleOut, size_t& offsetOut) {
    if (chunkCount < VBO::MAX_VBO_CHUNK_COUNT) {
        for (VBO& vbo : g_globalVBOs) {
            if (vbo.allocateChunks(chunkCount, usage, offsetOut)) {
                handleOut = vbo.handle();
                return true;
            }
        }

        VBO vbo;
        if (vbo.allocateChunks(chunkCount, usage, offsetOut)) {
            handleOut = vbo.handle();
            g_globalVBOs.push_back(vbo);
            return true;
        }
    } else {
        VBO vbo;
        if (vbo.allocateWhole(chunkCount, usage)) {
            offsetOut = 0;
            handleOut = vbo.handle();
            g_globalVBOs.push_back(vbo);
            return true;
        }
    }

    return false;
}

bool releaseVBO(GLuint& handle, size_t& offset) {
    for (VBO& vbo : g_globalVBOs) {
        if (vbo.handle() == handle) {
            vbo.releaseChunks(offset);
            handle = 0;
            offset = 0;
            return true;
        }
    }

    return false;
}

size_t getVBOMemUsage(const GLuint handle) {
    for (VBO& vbo : g_globalVBOs) {
        if (vbo.handle() == handle) {
            return vbo.getMemUsage();
        }
    }

    return 0;
}

U32 getVBOCount() noexcept {
    return to_U32(g_globalVBOs.size());
}

void clearVBOs() noexcept {
    g_globalVBOs.clear();
}

bufferPtr createAndAllocPersistentBuffer(const size_t bufferSize,
                                         const BufferStorageMask storageMask,
                                         const MapBufferAccessMask accessMask,
                                         GLuint& bufferIdOut,
                                         bufferPtr const data,
                                         const size_t dataSize,
                                         const char* name)
{
    assert(dataSize <= bufferSize);

    glCreateBuffers(1, &bufferIdOut);
    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_BUFFER,
                      bufferIdOut,
                      -1,
                      name != nullptr
                           ? name
                           : Util::StringFormat("DVD_PERSISTENT_BUFFER_%d", bufferIdOut).c_str());
    }
    assert(bufferIdOut != 0 && "GLUtil::allocPersistentBuffer error: buffer creation failed");

    glNamedBufferStorage(bufferIdOut, bufferSize, data, storageMask);
    const bufferPtr ptr = glMapNamedBufferRange(bufferIdOut, 0, bufferSize, accessMask);
    assert(ptr != nullptr);
    return ptr;
}

void createAndAllocBuffer(const size_t bufferSize,
                          const GLenum usageMask,
                          GLuint& bufferIdOut,
                          const bufferPtr data,
                          const size_t dataSize,
                          const bool initToZeroFreeSpace,
                          const char* name)
{
    assert(dataSize <= bufferSize);

    glCreateBuffers(1, &bufferIdOut);

    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_BUFFER,
                      bufferIdOut,
                      -1,
                      name != nullptr
                           ? name
                           : Util::StringFormat("DVD_GENERAL_BUFFER_%d", bufferIdOut).c_str());
    }

    assert(bufferIdOut != 0 && "GLUtil::allocBuffer error: buffer creation failed");
    glNamedBufferData(bufferIdOut, bufferSize, data, usageMask);

    if (initToZeroFreeSpace && dataSize < bufferSize) {
        const size_t sizeDiff = bufferSize - dataSize;
        const vectorEASTL<Byte> newData(sizeDiff, Byte{ 0 });
        glNamedBufferSubData(bufferIdOut, bufferSize - sizeDiff, sizeDiff, newData.data());
    }
}

void freeBuffer(GLuint& bufferId, bufferPtr mappedPtr) {
    if (bufferId > 0) {
        if (mappedPtr != nullptr) {
            const GLboolean result = glUnmapNamedBuffer(bufferId);
            assert(result != GL_FALSE && "GLUtil::freeBuffer error: buffer unmaping failed");
            ACKNOWLEDGE_UNUSED(result);
            mappedPtr = nullptr;
        }
        GL_API::deleteBuffers(1, &bufferId);
        bufferId = 0;
    }
}

};  // namespace GLUtil
};  // namespace Divide