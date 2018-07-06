#include "Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {
    namespace GLUtil {
    bufferPtr allocPersistentBuffer(GLuint bufferId, GLsizeiptr bufferSize,
                                    MapBufferUsageMask usageMask,
                                    BufferAccessMask accessMask) {

        STUBBED("Remove this hack when proper OpenGL4.5 support is available!")

        //glNamedBufferStorage(bufferId, bufferSize, NULL, usageMask);
        //return glMapNamedBufferRange(bufferId, 0, bufferSize, accessMask);

        gl45ext::glNamedBufferStorageEXT(bufferId, bufferSize, NULL, usageMask);
        bufferPtr ptr = gl45ext::glMapNamedBufferRangeEXT(bufferId, 0, bufferSize,
                                                 accessMask);
        
        if (!ptr) {
            GLuint previousBufferID = 0;
            GL_API::setActiveBuffer(GL_ARRAY_BUFFER, bufferId, previousBufferID);
            glBufferStorage(GL_ARRAY_BUFFER, bufferSize, NULL, usageMask);
            ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, accessMask);
            GL_API::setActiveBuffer(GL_ARRAY_BUFFER, previousBufferID);
        }

        return ptr;
    }

    bufferPtr allocPersistentBuffer(GLsizeiptr bufferSize,
                                    MapBufferUsageMask usageMask,
                                    BufferAccessMask accessMask,
                                    GLuint& bufferIdOut) {
        glGenBuffers(1, &bufferIdOut);
        DIVIDE_ASSERT(bufferIdOut != 0,
                  "GLUtil::allocPersistentBuffer error: buffer creation failed");
        return allocPersistentBuffer(bufferSize, bufferIdOut, usageMask, accessMask);
    }

    void allocBuffer(GLuint bufferId, GLsizeiptr bufferSize, GLenum usageMask) {
        // glNamedBufferData(bufferId, bufferSize, NULL, usageMask);
        gl45ext::glNamedBufferDataEXT(bufferId, bufferSize, NULL, usageMask);
    }

    void allocBuffer(GLsizeiptr bufferSize, GLenum usageMask,
                     GLuint& bufferIdOut) {
        glGenBuffers(1, &bufferIdOut);
        DIVIDE_ASSERT(bufferIdOut != 0,
                      "GLUtil::allocBuffer error: buffer creation failed");
        return allocBuffer(bufferSize, bufferIdOut, usageMask);
    }

    void freeBuffer(GLuint& bufferId, bufferPtr mappedPtr) {
        if (bufferId > 0)
        {
            if (mappedPtr != nullptr)
            {
                if (gl45ext::glUnmapNamedBufferEXT(bufferId) == GL_FALSE)
                {
                    GLuint previousBufferID = 0;
                    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, bufferId, previousBufferID);
                    GLboolean result = glUnmapBuffer(GL_ARRAY_BUFFER);
                    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, previousBufferID);

                    DIVIDE_ASSERT(result == GL_TRUE, "GLUtil::freePersistentBuffer error: can't unmap specified buffer!");
                                                      
                }
                mappedPtr = nullptr;
            }
            glDeleteBuffers(1, &bufferId);
        }
    }

    }; //namespace GLUtil
}; //namespace Divide