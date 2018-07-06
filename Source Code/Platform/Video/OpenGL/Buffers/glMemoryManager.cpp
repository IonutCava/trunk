#include "Headers/glMemoryManager.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"

namespace Divide {
namespace GLUtil {

bufferPtr allocPersistentBuffer(GLuint bufferId,
                                GLsizeiptr bufferSize,
                                MapBufferUsageMask usageMask,
                                BufferAccessMask accessMask,
                                const bufferPtr data) {
    STUBBED("Remove this hack when proper OpenGL4.5 support is available!")
#ifdef GL_VERSION_4_5
    glNamedBufferStorage(bufferId, bufferSize, data, usageMask);
    bufferPtr ptr = glMapNamedBufferRange(bufferId, 0, bufferSize, accessMask);
#else
    gl44ext::glNamedBufferStorageEXT(bufferId, bufferSize, data, usageMask);
    bufferPtr ptr =
        gl44ext::glMapNamedBufferRangeEXT(bufferId, 0, bufferSize, accessMask);
#endif
    if (!ptr) {
        GLuint previousBufferID = 0;
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, bufferId, previousBufferID);
        glBufferStorage(GL_ARRAY_BUFFER, bufferSize, data, usageMask);
        ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, accessMask);
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, previousBufferID);
    }

    return ptr;
}

bufferPtr createAndAllocPersistentBuffer(GLsizeiptr bufferSize,
                                         MapBufferUsageMask usageMask,
                                         BufferAccessMask accessMask,
                                         GLuint& bufferIdOut,
                                         bufferPtr const data) {
    glGenBuffers(1, &bufferIdOut);
    DIVIDE_ASSERT(
        bufferIdOut != 0,
        "GLUtil::allocPersistentBuffer error: buffer creation failed");

    return allocPersistentBuffer(bufferIdOut, bufferSize, usageMask, accessMask,
                                 data);
}

void allocBuffer(GLuint bufferId,
                 GLsizeiptr bufferSize,
                 GLenum usageMask,
                 const bufferPtr data) {
#ifdef GL_VERSION_4_5
    glNamedBufferData(bufferId, bufferSize, data, usageMask);
#else
    gl44ext::glNamedBufferDataEXT(bufferId, bufferSize, data, usageMask);
#endif
}

void createAndAllocBuffer(GLsizeiptr bufferSize,
                          GLenum usageMask,
                          GLuint& bufferIdOut,
                          const bufferPtr data) {
    glGenBuffers(1, &bufferIdOut);
    DIVIDE_ASSERT(bufferIdOut != 0,
                  "GLUtil::allocBuffer error: buffer creation failed");
    return allocBuffer(bufferIdOut, bufferSize, usageMask, data);
}

void updateBuffer(GLuint bufferId,
                  GLintptr offset,
                  GLsizeiptr size,
                  const bufferPtr data) {
#ifdef GL_VERSION_4_5
    glNamedBufferSubData(bufferId, offset, size, data);
#else
    gl44ext::glNamedBufferSubDataEXT(bufferId, offset, size, data);
#endif
}

void freeBuffer(GLuint& bufferId, bufferPtr mappedPtr) {
    if (bufferId > 0) {
        if (mappedPtr != nullptr) {
#ifdef GL_VERSION_4_5
            GLboolean result = glUnmapNamedBuffer(bufferId);
#else
            GLboolean result = gl44ext::glUnmapNamedBufferEXT(bufferId);
#endif
            if (result == GL_FALSE) {
                GLuint previousBufferID = 0;
                GL_API::setActiveBuffer(GL_ARRAY_BUFFER, bufferId,
                                        previousBufferID);
                GLboolean result = glUnmapBuffer(GL_ARRAY_BUFFER);
                GL_API::setActiveBuffer(GL_ARRAY_BUFFER, previousBufferID);

                DIVIDE_ASSERT(result == GL_TRUE,
                              "GLUtil::freePersistentBuffer error: "
                              "can't unmap specified buffer!");
            }
            mappedPtr = nullptr;
        }
        glDeleteBuffers(1, &bufferId);
        bufferId = 0;
    }
}

};  // namespace GLUtil
};  // namespace Divide