#include "Headers/glGenericBuffer.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glBufferImpl.h"

namespace Divide {
glGenericBuffer::glGenericBuffer(GLenum usage, bool persistentMapped)
    : _elementCount(0),
      _elementSize(0),
      _feedbackBindPoint(-1)
{
    if (persistentMapped) {
        _buffer = MemoryManager_NEW glPersistentBuffer(usage);
    } else {
        _buffer = MemoryManager_NEW glRegularBuffer(usage);
    }
}

glGenericBuffer::~glGenericBuffer()
{
    _buffer->destroy();
    MemoryManager::DELETE(_buffer);
}

}; //namespace Divide