/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _GL_MEMORY_MANAGER_H_
#define _GL_MEMORY_MANAGER_H_

#include "Platform/Video/OpenGL/Headers/glResources.h"

namespace Divide {
namespace GLUtil {
    class VBO {
    public:
        // Allocate VBOs in 64K chunks. This will HIGHLY depend on actual data usage and requires testing.
        static const U32 MAX_VBO_CHUNK_SIZE_BYTES = 64 * 1024;
        // nVidia recommended (years ago) to use up to 4 megs per VBO. Use 32 MEGS VBOs :D
        static const U32  MAX_VBO_SIZE_BYTES = 32 * 1024 * 1024;
        // The total number of available chunks per VBO is easy to figure out
        static const U32 MAX_VBO_CHUNK_COUNT = MAX_VBO_SIZE_BYTES / MAX_VBO_CHUNK_SIZE_BYTES;

        //keep track of what chunks we are using
        //for each chunk, keep track how many next chunks are also part of the same allocation
        std::array<std::pair<bool, U32>, MAX_VBO_CHUNK_COUNT> _chunkUsageState;

        static U32 getChunkCountForSize(size_t sizeInBytes);

        VBO() noexcept;
        ~VBO();

        void freeAll();
        U32 handle();
        bool checkChunksAvailability(size_t offset, U32 count);

        bool allocateChunks(U32 count, GLenum usage, size_t& offsetOut);

        bool allocateWhole(U32 count, GLenum usage);

        void releaseChunks(size_t offset);

        U32 getMemUsage();

    private:
        GLuint _handle;
        GLenum _usage;
        bool   _filledManually;
    };

    struct AllocationHandle {
        explicit AllocationHandle()
            : _id(0),
              _offset(0)
        {
        }

        GLuint _id;
        size_t _offset;
    };

    bool commitVBO(U32 chunkCount, GLenum usage, GLuint& handleOut, size_t& offsetOut);
    bool releaseVBO(GLuint& handle, size_t& offset);
    U32 getVBOMemUsage(GLuint handle);
    U32 getVBOCount();

    void clearVBOs();

    void createAndAllocBuffer(GLsizeiptr bufferSize,
                              GLenum usageMask,
                              GLuint& bufferIdOut,
                              const bufferPtr data,
                              const char* name = nullptr);

    bufferPtr allocPersistentBuffer(GLuint bufferId,
                                    GLsizeiptr bufferSize,
                                    BufferStorageMask storageMask,
                                    BufferAccessMask accessMask,
                                    const bufferPtr data);

    bufferPtr createAndAllocPersistentBuffer(GLsizeiptr bufferSize,
                                             BufferStorageMask storageMask,
                                             BufferAccessMask accessMask,
                                             GLuint& bufferIdOut,
                                             const bufferPtr data,
                                             const char* name = nullptr);

    void freeBuffer(GLuint &bufferId, bufferPtr mappedPtr = nullptr);

}; //namespace GLUtil
}; //namespace Divide

#endif //_GL_MEMORY_MANAGER_H_