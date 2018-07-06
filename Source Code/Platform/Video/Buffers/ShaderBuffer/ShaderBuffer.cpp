#include "Headers/ShaderBuffer.h"

namespace Divide {
    ShaderBuffer::ShaderBuffer(const stringImpl& bufferName,
                               const U32 sizeFactor,
                               bool unbound,
                               bool persistentMapped) : RingBuffer(sizeFactor),
                                                        GUIDWrapper(),
                                                        _bufferSize(0),
                                                        _primitiveSize(0),
                                                        _primitiveCount(0),
                                                        _unbound(unbound),
                                                        _persistentMapped(persistentMapped &&
                                                            !Config::Profile::DISABLE_PERSISTENT_BUFFER)
    {
    }

    ShaderBuffer::~ShaderBuffer()
    {
    }

    void ShaderBuffer::Create(U32 primitiveCount, ptrdiff_t primitiveSize) {
        _primitiveCount = primitiveCount;
        _primitiveSize = primitiveSize;
        _bufferSize = primitiveSize * primitiveCount;
        DIVIDE_ASSERT(_bufferSize > 0, "ShaderBuffer::Create error: Invalid buffer size!");
    }

    void ShaderBuffer::SetData(const bufferPtr data) {
        DIVIDE_ASSERT(_bufferSize > 0, "ShaderBuffer::SetData error: Invalid buffer size!");

        U32 count = queueLength();
        for (U32 i = 0; i < count; ++i) {
            UpdateData(0, _primitiveCount, data);
            incQueue();
        }
    }

}; //namespace Divide;