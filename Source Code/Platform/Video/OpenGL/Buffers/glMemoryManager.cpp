#include "Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"

namespace Divide {
namespace GLUtil {


static vectorImpl<VBO> g_globalVBOs;

U32 VBO::getChunkCountForSize(size_t sizeInBytes) {
    return to_uint(std::ceil(to_float(sizeInBytes) / MAX_VBO_CHUNK_SIZE_BYTES));
}

VBO::VBO() : _handle(0),
             _usage(GL_NONE),
             _filledManually(false)
{
    _chunkUsageState.fill(std::make_pair(false, 0));
}

VBO::~VBO()
{
}

void VBO::freeAll() {
    if (_handle != 0) {
        GLUtil::freeBuffer(_handle);
        _handle = 0;
    }
    _usage = GL_NONE;
}

U32 VBO::handle() {
    return _handle;
}

bool VBO::checkChunksAvailability(U32 offset, U32 count) {
    std::pair<bool, U32>& chunk = _chunkUsageState[offset];
    U32 freeChunkCount = 0;
    if (!chunk.first) {
        freeChunkCount++;
        for (U32 j = 1; j < MAX_VBO_CHUNK_COUNT - offset; ++j) {
            std::pair<bool, U32>& chunkChild = _chunkUsageState[offset + j];
            if (chunkChild.first) {
                break;
            } else {
                freeChunkCount++;
            }
        }
    }

    return freeChunkCount >= count;
}

bool VBO::allocateChunks(U32 count, GLenum usage, U32& offsetOut) {
    if (_usage == GL_NONE || _usage == usage) {
        for (U32 i = 0; i < MAX_VBO_CHUNK_COUNT; ++i) {
            if (checkChunksAvailability(i, count)) {
                if (_handle == 0) {
                    GLUtil::createAndAllocBuffer(MAX_VBO_SIZE_BYTES, usage, _handle);
                    _usage = usage;
                }
                offsetOut = i;
                _chunkUsageState[i].first = true;
                _chunkUsageState[i].second = count;
                for (U32 j = 1; j < count; ++j) {
                    _chunkUsageState[j + i].first = true;
                }
                return true;
            }
        }
    }

    return false;
}

bool VBO::allocateWhole(U32 count, GLenum usage) {
    assert(_handle == 0);
    GLUtil::createAndAllocBuffer(count * MAX_VBO_CHUNK_SIZE_BYTES, usage, _handle);
    _usage = usage;
    _chunkUsageState.fill(std::make_pair(true, 0));
    _chunkUsageState[0].second = count;
    _filledManually = true;
    return true;
}

void VBO::releaseChunks(U32 offset) {
    if (_filledManually) {
        _chunkUsageState.fill(std::make_pair(false, 0));
        return;
    }

    assert(offset < MAX_VBO_CHUNK_COUNT);
    assert(_chunkUsageState[offset].second != 0);
    U32 childCount = _chunkUsageState[offset].second;
    for (U32 i = 0; i < childCount; ++i) {
        std::pair<bool, U32>& chunkChild = _chunkUsageState[i + offset];
        assert(chunkChild.first);
        chunkChild.first = false;
        chunkChild.second = 0;
    }
}

U32 VBO::getMemUsage() {
    U32 usedBlocks = 0;
    for (std::pair<bool, U32>& chunk : _chunkUsageState) {
        if (chunk.first) {
            usedBlocks++;
        }
    }

    return usedBlocks * MAX_VBO_CHUNK_SIZE_BYTES;
}

bool commitVBO(U32 chunkCount, GLenum usage, GLuint& handleOut, U32& offsetOut) {
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

bool releaseVBO(GLuint& handle, U32& offset) {
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

U32 getVBOMemUsage(GLuint handle) {
    for (VBO& vbo : g_globalVBOs) {
        if (vbo.handle() == handle) {
            return vbo.getMemUsage();
        }
    }

    return 0;
}

U32 getVBOCount() {
    return to_uint(g_globalVBOs.size());
}

void clearVBOs() {
    g_globalVBOs.clear();
}

bufferPtr allocPersistentBuffer(GLuint bufferId,
                                GLsizeiptr bufferSize,
                                BufferStorageMask storageMask,
                                BufferAccessMask accessMask,
                                const bufferPtr data) {
    glNamedBufferStorage(bufferId, bufferSize, data, storageMask);
    bufferPtr ptr = glMapNamedBufferRange(bufferId, 0, bufferSize, accessMask);
    assert(ptr != NULL);
    return ptr;
}

bufferPtr createAndAllocPersistentBuffer(GLsizeiptr bufferSize,
                                         BufferStorageMask storageMask,
                                         BufferAccessMask accessMask,
                                         GLuint& bufferIdOut,
                                         bufferPtr const data) {
    glCreateBuffers(1, &bufferIdOut);
    assert(bufferIdOut != 0 && "GLUtil::allocPersistentBuffer error: buffer creation failed");

    return allocPersistentBuffer(bufferIdOut, bufferSize, storageMask, accessMask,
                                 data);
}

void createAndAllocBuffer(GLsizeiptr bufferSize,
                          GLenum usageMask,
                          GLuint& bufferIdOut,
                          const bufferPtr data) {
    glCreateBuffers(1, &bufferIdOut);
    assert(bufferIdOut != 0 && "GLUtil::allocBuffer error: buffer creation failed");
    glNamedBufferData(bufferIdOut, bufferSize, data, usageMask);
}

void freeBuffer(GLuint& bufferId, bufferPtr mappedPtr) {
    if (bufferId > 0) {
        if (mappedPtr != nullptr) {
            GLboolean result = glUnmapNamedBuffer(bufferId);
            assert(result != GL_FALSE && "GLUtil::freeBuffer error: buffer unmaping failed");
            ACKNOWLEDGE_UNUSED(result);
            mappedPtr = nullptr;
        }
        glDeleteBuffers(1, &bufferId);
        bufferId = 0;
    }
}

};  // namespace GLUtil
};  // namespace Divide